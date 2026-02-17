#include "asset_manager.hpp"
#include "resources/pipeline_state_loader.hpp"
#include "resources/texture2d_loader.hpp"
#include "resources/material_loader.hpp"
#include "resources/model_loader.hpp"

#include <fstream>

AssetManager::AssetManager() : device(nullptr), assetDir("assets/") {}

AssetManager::~AssetManager() {}

bool AssetManager::Initialize(ID3D11Device *device) {
    LogInfo("Creating asset manager...\n");
    LogIndent();

    this->device = device;

    this->registry.SetAssetDirectory(this->assetDir);
    this->registry.RegisterAssets();

    this->RegisterAssetType<Pipeline_state, PipelineStateLoader>();
    this->RegisterAssetType<Texture2D, Texture2DLoader>();
    this->RegisterAssetType<Material, MaterialLoader>();
    this->RegisterAssetType<Model, ModelLoader>();

    LogUnindent();

    return true;
}

// Ignores current asset directory
bool AssetManager::LoadFileContents(const std::string &path, void **buffer, size_t *size) {
    if (!buffer || !size)
        return false;

    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        LogWarn("Failed to open file '%s'\n", path.c_str());
        return false;
    }

    *size = file.tellg();
    *buffer = malloc(*size);

    if (!*buffer) {
        LogWarn("Failed to allocate memory for file '%s'\n", path.c_str());
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read((char *)*buffer, *size);
    file.close();

    return true;
}

std::string AssetManager::UUIDToPath(AssetID uuid) {
    return this->registry.GetPath(uuid);
}

AssetID AssetManager::PathToUUID(const std::string &path) {
    return this->registry.GetUUID(path);
}

void AssetManager::MarkAllAssetsUnused() {
    for (auto &[typeIndex, cache] : this->caches)
        cache->MarkAllUnused();
}

void AssetManager::MarkAsUsed(AssetID uuid) {
    for (auto &[typeIndex, cache] : this->caches)
        cache->MarkAsUsed(uuid);
}

void AssetManager::CleanUpUnused() {
    for (auto &[typeIndex, cache] : this->caches)
        cache->CleanUpUnused();
}

void AssetManager::SetAssetDirectory(const std::string &path) {
    this->assetDir = path;
}

const std::string &AssetManager::GetAssetDirectory() const {
    return this->assetDir;
}

ID3D11Device *AssetManager::GetDevice() {
    return this->device;
}