#include "material_loader.hpp"
#include "resources/asset_manager.hpp"
#include "resources/texture2d_loader.hpp"

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

    Texture2DLoader loader(this->assetManager);
    loader.SetDevice(this->device);
    loader.SetDeviceContext(this->deviceContext);

    const UINT width  = 1;
    const UINT height = 1;
    UINT32 pixels[width * height] = {
        0xFFFF8080
    };

    Texture2D *normalTexture = loader.CreateFromBitmap(pixels, width, height, false);

    material->normalTexture = this->assetManager->AddAsset<Texture2D>(normalTexture, true);

    LogInfo("Default Material asset created\n");

    return material;
}