#include "material_loader.hpp"
#include "resources/asset_manager.hpp"

Material *MaterialLoader::Load(const std::string &path) {
    return nullptr;
}

Material *MaterialLoader::CreateDefault() {
    Material *material = new Material();

    material->pipelineState = this->assetManager->GetHandle<Pipeline_state>(AssetID::invalid);
    material->diffuseTexture = this->assetManager->GetHandle<Texture2D>(AssetID::invalid);
    material->ambientColour = {1.0f, 1.0f, 1.0f};
    material->diffuseColour = {1.0f, 1.0f, 1.0f};
    material->specularColour = {1.0f, 1.0f, 1.0f};
    material->specularExponent = 32.0f;

    // TODO: "Copy asset" method?
    const UINT width  = 1;
    const UINT height = 1;
    UINT32 pixels[width * height] = {
        0xFFFF8080
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
        LogWarn("Failed to create default normal Texture2D\n");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    Texture2D *normalTexture = new Texture2D();

    result = this->device->CreateShaderResourceView(texture, &shaderResourceViewDesc, 
        &normalTexture->shaderResourceView);

    texture->Release();

    if (FAILED(result)) {
        delete normalTexture;
        LogWarn("Failed to create default normal Texture2D SRV\n");
        return nullptr;
    }

    normalTexture->width = width;
    normalTexture->height = height;

    material->normalTexture = this->assetManager->AddAsset<Texture2D>(normalTexture, true);

    LogInfo("Default Material asset created\n");

    return material;
}