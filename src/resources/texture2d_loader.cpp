#include "texture2d_loader.hpp"
#include "core/logging.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

Texture2D *Texture2DLoader::Load(const std::string &path) {
    int width, height, components;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &components, 4);

    if (!data) {
        LogWarn("Failed to load image file '%s': '%s'\n", path.c_str(), stbi_failure_reason());
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA inititialData{};
    inititialData.pSysMem = data;
    inititialData.SysMemPitch = width * sizeof(unsigned char) * 4;
    inititialData.SysMemSlicePitch = 0;

    ID3D11Texture2D *texture{};
    HRESULT result = this->device->CreateTexture2D(&textureDesc, &inititialData, &texture);

    stbi_image_free(data);

    if (FAILED(result)) {
        LogWarn("Failed to create texture '%s'\n", path.c_str());
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    Texture2D *texture2D = new Texture2D();
    result = this->device->CreateShaderResourceView(texture, &shaderResourceViewDesc, 
        &texture2D->shaderResourceView);

    texture->Release();

    if (FAILED(result)) {
        LogWarn("Failed to create SRV for texture '%s'\n", path.c_str());
        delete texture2D;
        return nullptr;
    }

    texture2D->width = width;
    texture2D->height = height;

    LogInfo("Texture2D '%s' loaded successfully (%dx%d)\n", path.c_str(), width, height);

    return texture2D;
}

Texture2D *Texture2DLoader::CreateDefault() {
    const UINT width  = 1;
    const UINT height = 1;
    UINT32 pixels[width * height] = {
        0xFFFFFFFF
    };

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = pixels;
    initialData.SysMemPitch = width * sizeof(UINT32);
    initialData.SysMemSlicePitch = 0;

    ID3D11Texture2D *texture{};
    HRESULT result = this->device->CreateTexture2D(&textureDesc, &initialData, &texture);
    if (FAILED(result)) {
        LogWarn("Failed to create default Texture2D\n");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    Texture2D *texture2D = new Texture2D();

    result = this->device->CreateShaderResourceView(texture, &shaderResourceViewDesc, 
        &texture2D->shaderResourceView);

    texture->Release();

    if (FAILED(result)) {
        delete texture2D;
        LogWarn("Failed to create default Texture2D SRV\n");
        return nullptr;
    }

    texture2D->width = width;
    texture2D->height = height;

    LogInfo("Default Texture2D asset created\n");

    return texture2D;
}