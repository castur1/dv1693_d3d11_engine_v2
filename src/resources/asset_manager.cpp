#include "asset_manager.hpp"
#include "core/logging.hpp"

AssetManager::AssetManager() : device(nullptr), assetDir("assets/") {}

AssetManager::~AssetManager() {}

bool AssetManager::Initialize(ID3D11Device *device) {
    LogInfo("Creating asset manager...\n");
    LogIndent();

    this->device = device;

    // this->RegisterAssets();

    // this->CreateDefaultTexture2D();

    LogUnindent();

    return true;
}

// TODO
void AssetManager::MarkAllAssetsUnused() {

}

void AssetManager::MarkAsUsed(AssetID uuid) {

}

void AssetManager::CleanUpUnused() {

}


void AssetManager::SetAssetDirectory(const std::string &path) {
    this->assetDir = path;
}