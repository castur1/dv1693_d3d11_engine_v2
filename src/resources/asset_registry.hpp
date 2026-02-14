#ifndef ASSET_REGISTRY_HPP
#define ASSET_REGISTRY_HPP

#include "core/uuid.hpp"

#include <string>
#include <unordered_map>

class AssetRegistry {
    std::unordered_map<AssetID, std::string> uuidToPath;
    std::unordered_map<std::string, AssetID> pathToUUID;

    std::string assetDir;

public:
    AssetRegistry() = default;
     ~AssetRegistry() = default;

    void RegisterAssets();
    void Clear();

    std::string GetPath(AssetID uuid);
    AssetID GetUUID(const std::string &path);

    void SetAssetDirectory(const std::string &path);
};

#endif