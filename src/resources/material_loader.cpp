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

    LogInfo("Default Material asset created\n");

    return material;
}