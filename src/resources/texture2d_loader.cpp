#include "texture2d_loader.hpp"
#include "core/logging.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

// TODO: Description as argument?
Texture2D *Texture2DLoader::CreateFromBitmap(UINT32 *pixels, UINT width, UINT height, bool generateMips) {
    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.CPUAccessFlags = 0;

    if (generateMips) {
        textureDesc.MipLevels = 0;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    else {
        textureDesc.MipLevels = 1;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.MiscFlags = 0;
    }

    ID3D11Texture2D *texture{};
    HRESULT result = this->device->CreateTexture2D(&textureDesc, nullptr, &texture);
    if (FAILED(result)) {
        LogWarn("Failed to create Texture2D\n");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = -1;

    Texture2D *texture2D = new Texture2D();

    result = this->device->CreateShaderResourceView(
        texture, 
        &shaderResourceViewDesc, 
        &texture2D->shaderResourceView
    );

    if (FAILED(result)) {
        delete texture2D;
        texture->Release();
        LogWarn("Failed to create Texture2D SRV\n");
        return nullptr;
    }

    this->deviceContext->UpdateSubresource(texture, 0, nullptr, pixels, width * sizeof(UINT32), 0);
    if (generateMips)
        this->deviceContext->GenerateMips(texture2D->shaderResourceView);

    texture->Release();

    texture2D->width  = width;
    texture2D->height = height;

    return texture2D;
}

Texture2D *Texture2DLoader::Load(const std::string &path) {
    int width, height, components;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &components, 4);

    if (!data) {
        LogWarn("Failed to load image file '%s': '%s'\n", path.c_str(), stbi_failure_reason());
        return nullptr;
    }

    Texture2D *texture2D = this->CreateFromBitmap((UINT32 *)data, width, height);

    stbi_image_free(data);

    LogInfo("Texture2D '%s' loaded successfully (%dx%d)\n", path.c_str(), width, height);

    return texture2D;
}

Texture2D *Texture2DLoader::CreateDefault() {
    const UINT width  = 1;
    const UINT height = 1;
    UINT32 pixels[width * height] = {
        0xFFFFFFFF
    };

    Texture2D *texture2D = this->CreateFromBitmap(pixels, width, height);

    LogInfo("Default Texture2D asset created\n");

    return texture2D;
}