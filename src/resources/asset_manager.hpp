#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "core/uuid.hpp"
#include "resources/asset_cache.hpp"
#include "resources/asset_registry.hpp"
#include "resources/asset_loader.hpp"
#include "core/logging.hpp"

#include <d3d11.h>
#include <string>
#include <typeindex>

class AssetManager;

template <typename T>
class AssetHandle {
    AssetID uuid;
    T *cachedPtr;

    AssetManager *assetManager;

public:
    AssetHandle() : uuid(AssetID::invalid), cachedPtr(nullptr), assetManager(nullptr) {}

    AssetHandle(const AssetID &uuid, AssetManager *assetManager)
        : uuid(uuid), cachedPtr(nullptr), assetManager(assetManager) {}

    ~AssetHandle() = default;

    T *Get();
};

class AssetManager {
    ID3D11Device *device;

    AssetRegistry registry;

    std::unordered_map<std::type_index, std::unique_ptr<AssetCacheBase>> caches;
    std::unordered_map<std::type_index, std::unique_ptr<AssetLoaderBase>> loaders;

    std::string assetDir;

    template <typename T>
    AssetCache<T> *GetCache() {
        auto iter = this->caches.find(std::type_index(typeid(T)));
        if (iter == this->caches.end())
            return nullptr;

        return static_cast<AssetCache<T> *>(iter->second.get());
    }

    template <typename T>
    AssetLoader<T> *GetLoader() {
        auto iter = this->loaders.find(std::type_index(typeid(T)));
        if (iter == this->loaders.end())
            return nullptr;

        return static_cast<AssetLoader<T> *>(iter->second.get());
    }

public:
    AssetManager();
    ~AssetManager();

    bool Initialize(ID3D11Device *device);

    // LoaderType implements AssetLoader<T>
    template <typename T, typename LoaderType>
    void RegisterAssetType() {
        auto typeIndex = std::type_index(typeid(T));

        auto cache = std::make_unique<AssetCache<T>>();
        auto loader = std::make_unique<LoaderType>(this);

        loader->SetDevice(this->device);
        cache->SetDefault(loader->CreateDefault());

        this->caches[typeIndex] = std::move(cache);
        this->loaders[typeIndex] = std::move(loader);
    }

    template <typename T>
    T *Load(const AssetID &uuid) {
        auto *cache = this->GetCache<T>();
        if (!cache) {
            LogWarn("Unknown asset type\n");
            return nullptr;
        }

        if (!uuid.IsValid())
            return cache->GetDefault();

        if (cache->Contains(uuid))
            return cache->Get(uuid);

        auto *loader = this->GetLoader<T>();
        if (!loader)
            return cache->GetDefault();

        std::string path = this->registry.GetPath(uuid);
        if (path.empty())
            return cache->GetDefault();

        T *asset = loader->Load(this->assetDir + path);
        if (!asset) {
            delete asset;
            return cache->GetDefault();
        }

        return cache->Add(uuid, asset);
    }

    template <typename T>
    T *Load(const std::string &path) {
        AssetID uuid = this->registry.GetUUID(path);
        return this->Load<T>(uuid);
    }

    template <typename T>
    bool Has(const AssetID &uuid) {
        auto *cache = this->GetCache<T>();
        if (!cache)
            return false;

        return cache->Contains(uuid);
    }

    template <typename T>
    T *GetDefault() {
        auto *cache = this->GetCache<T>();
        if (!cache)
            return nullptr;

        return cache->GetDefault();
    }

    template <typename T>
    AssetHandle<T> AddAsset(T *asset, AssetID uuid) {
        auto *cache = this->GetCache<T>();
        if (!cache)
            return AssetHandle<T>(AssetID::invalid, this);

        cache->Add(uuid, asset);
        return AssetHandle<T>(uuid, this);
    }

    template <typename T>
    AssetHandle<T> AddAsset(T *asset) {
        AssetID uuid;
        return this->AddAsset<T>(asset, uuid);
    }

    template <typename T>
    AssetHandle<T> GetHandle(const AssetID &uuid) {
        return AssetHandle<T>(uuid, this);
    }

    void MarkAllAssetsUnused();
    void MarkAsUsed(AssetID uuid);
    void CleanUpUnused();

    bool LoadFileContents(const std::string &path, void **buffer, size_t *size);

    std::string UUIDToPath(AssetID uuid);
    AssetID PathToUUID(const std::string &path);

    void SetAssetDirectory(const std::string &path);
    const std::string &GetAssetDirectory() const;

    ID3D11Device *GetDevice();
};

template <typename T>
T *AssetHandle<T>::Get() {
    if (cachedPtr)
        return cachedPtr;

    if (!assetManager)
        return nullptr;

    return this->cachedPtr = this->assetManager->Load<T>(this->uuid);
}

#endif