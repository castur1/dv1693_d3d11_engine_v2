#ifndef ASSET_REGISTRY_HPP
#define ASSET_REGISTRY_HPP

#include "core/uuid.hpp"

#include <string>
#include <unordered_map>

class AssetRegistry {
    std::unordered_map<AssetID, std::string> uuidToPath;
    std::unordered_map<std::string, AssetID> pathToUUID;

    std::unordered_map<AssetID, std::unordered_map<std::string, std::string>> metadata;

    std::string assetDir;

public:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    void RegisterAssets();
    void Clear();

    std::string GetPath(AssetID uuid) const;
    AssetID GetUUID(const std::string &path) const;

    std::string GetMetadata(AssetID uuid, const std::string &key, const std::string &defaultValue = "") const;
    void SetMetadata(AssetID uuid, const std::string &key, const std::string &value);

    void SetAssetDirectory(const std::string &path);
};

#endif