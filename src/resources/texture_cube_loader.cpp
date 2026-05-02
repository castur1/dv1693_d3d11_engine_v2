#include "texture_cube_loader.hpp"
#include "core/logging.hpp"

#include "stb_image/stb_image.h"

// +x, -x, +y, -y, +z, -z
static constexpr const char *FACE_SUFFIXES[6] = {"_px", "_nx", "_py", "_ny", "_pz", "_nz"};

static bool GetPathBase(const std::string &path, std::string &outBase, std::string &outExtension) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        LogWarn("Path '%s' has no extension\n", path.c_str());
        return false;
    }

    outExtension = path.substr(dot);
    std::string withoutExtension = path.substr(0, dot);

    if (withoutExtension.size() < 3 || withoutExtension.substr(withoutExtension.size() - 3) != "_px") {
        LogWarn("File name '%s' does not end with '_px'\n", withoutExtension.c_str());
        return false;
    }

    outBase = withoutExtension.substr(0, withoutExtension.size() - 3);

    return true;
}

Texture_cube *TextureCubeLoader::Load(const std::string &path) {
    std::string base, extension;
    if (!GetPathBase(path, base, extension))
        return nullptr;

    UINT32 *pixels[6] = {};
    int resolution = 0;

    bool didSucceed = true;

    for (int i = 0; i < 6; ++i) {
        std::string facePath = base + FACE_SUFFIXES[i] + extension;

        int width, height, components;
        pixels[i] = (UINT32 *)stbi_load(path.c_str(), &width, &height, &components, 4);

        if (!pixels[i]) {
            LogWarn("Failed to load image file '%s': '%s'\n", facePath.c_str(), stbi_failure_reason());
            didSucceed = false;
            break;
        }

        if (width != height) {
            LogWarn("Image '%s' is not square (%dx%d)", facePath.c_str(), width, height);
            didSucceed = false;
            break;
        }

        if (i == 0)
            resolution = width;
        else if (width != resolution) {
            LogWarn("Image '%s' has the wrong resolution (%dx%d instead of %dx%d)", facePath.c_str(), width, width, resolution, resolution);
            didSucceed = false;
            break;
        }
    }

    Texture_cube *textureCube = nullptr;
    if (didSucceed)
        this->CreateFromBitmaps(pixels, resolution);

    for (int i = 0; i < 6 && pixels[i]; ++i)
        stbi_image_free(pixels[i]);

    if (textureCube)
        LogInfo("Texture_cube '%s' loaded successfully (%dx%d)\n", base.c_str(), resolution, resolution);

    return textureCube;
}

Texture_cube *TextureCubeLoader::CreateDefault() {
    UINT32 black[1 * 1] = {0xFF000000};
    UINT32 *pixels[6] = {black, black, black, black, black, black};

    Texture_cube *textureCube = this->CreateFromBitmaps(pixels, 1);

    LogInfo("Default Texture_cube asset created\n");

    return textureCube;
}

Texture_cube *TextureCubeLoader::CreateFromBitmaps(UINT32 *pixels[6], UINT resolution) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = resolution;
    desc.Height = resolution;
    desc.MipLevels = 0;
    desc.ArraySize = 6;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

    ID3D11Texture2D *texture = nullptr;
    HRESULT result = this->device->CreateTexture2D(&desc, nullptr, &texture);
    if (FAILED(result)) {
        LogWarn("Failed to create texture2D array\n");
        return nullptr;
    }

    texture->GetDesc(&desc);
    UINT mipLevels = desc.MipLevels;

    for (int i = 0; i < 6; ++i) {
        UINT subresource = D3D11CalcSubresource(0, i, mipLevels);
        this->deviceContext->UpdateSubresource(texture, subresource, nullptr, pixels[i], resolution * sizeof(UINT32), 0);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = -1;

    Texture_cube *textureCube = new Texture_cube();

    result = this->device->CreateShaderResourceView(texture, &srvDesc, &textureCube->shaderResourceView);
    if (FAILED(result)) {
        delete textureCube;
        texture->Release();
        LogWarn("Failed to create SRV\n");
        return nullptr;
    }

    this->deviceContext->GenerateMips(textureCube->shaderResourceView);

    texture->Release();

    textureCube->resolution = resolution;

    return textureCube;
}
