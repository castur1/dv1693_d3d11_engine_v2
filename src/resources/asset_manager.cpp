#include "asset_manager.hpp"
#include "core/logging.hpp"

AssetManager::AssetManager() : device(nullptr), assetDir("assets/") {}

AssetManager::~AssetManager() {}

bool AssetManager::Initialize(ID3D11Device *device) {
    LogInfo("Creating asset manager...\n");
    LogIndent();

    this->device = device;

    this->assetRegistry.SetAssetDirectory(this->assetDir);
    this->assetRegistry.RegisterAssets();

    // this->CreateDefaultTexture2D();

    LogUnindent();

    return true;
}

void AssetManager::MarkAllAssetsUnused() {
    this->modelCache.MarkAllUnused();
    this->texture2DCache.MarkAllUnused();
    this->materialCache.MarkAllUnused();
    this->pipelineStateCache.MarkAllUnused();
}

void AssetManager::MarkAsUsed(AssetID uuid) {
    this->modelCache.MarkAsUsed(uuid);
    this->texture2DCache.MarkAsUsed(uuid);
    this->materialCache.MarkAsUsed(uuid);
    this->pipelineStateCache.MarkAsUsed(uuid);
}

void AssetManager::CleanUpUnused() {
    this->modelCache.CleanUpUnused();
    this->texture2DCache.CleanUpUnused();
    this->materialCache.CleanUpUnused();
    this->pipelineStateCache.CleanUpUnused();
}

void AssetManager::SetAssetDirectory(const std::string &path) {
    this->assetDir = path;
}