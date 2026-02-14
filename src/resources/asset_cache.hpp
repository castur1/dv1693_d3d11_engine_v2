#ifndef ASSET_CACHE_HPP
#define ASSET_CACHE_HPP

#include "core/uuid.hpp"
#include "core/logging.hpp"

#include <memory>
#include <unordered_map>

template <typename T>
class AssetCache {
    struct Entry {
        std::unique_ptr<T> asset;
        bool isUsed;
        bool isPersistent;
    };

    std::unordered_map<AssetID, Entry> cache;
    std::unique_ptr<T> defaultAsset;

public:
    AssetCache() = default;
    ~AssetCache() = default;

    T *Get(AssetID uuid) {
        auto iter = this->cache.find(uuid);
        if (iter == this->cache.end())
            return this->defaultAsset.get();

        return iter->second.get();
    }

    bool Contains(AssetID uuid) {
        return this->cache.contains(uuid);
    }

    T *Add(AssetID uuid, T *asset, bool isPersistent = false) {
        Entry entry{};
        entry.asset = std::make_unique<T>(asset);
        entry.isUsed = true;
        entry.isPersistent = isPersistent;

        auto [iter, wasInserted] = this->cache.insert_or_assign(uuid, std::move(entry));
        return iter->second.asset.get();
    }

    void Remove(AssetID uuid) {
        auto iter = this->cache.find(uuid);
        if (iter != this->cache.end())
            this->cache.erase(iter);
    }

    void Clear() {
        this->cache.clear();
    }

    void SetDefaultAsset(T *asset) {
        this->defaultAsset = std::make_unique<T>(asset);
    }

    T *GetDefaultAsset() {
        return this->defaultAsset.get();
    }

    void MarkAllUnused() {
        for (auto &asset : this->cache)
            asset.second.isUsed = false;
    }

    void MarkAsUsed(AssetID uuid) {
        auto iter = this->cache.find(uuid);
        if (iter != this->cache.end())
            iter->second.isUsed = true;
    }

    void CleanUpUnused() {
        for (auto iter = this->cache.begin(); iter != this->cache.end();) {
            if (iter->second.isUsed || iter->second.isPersistent) {
                ++iter;
                continue;
            }

            LogInfo("Unloading unused asset '%s'\n", iter->first.ToString().c_str());
            iter = this->cache.erase(iter);
        }
    }
};

#endif