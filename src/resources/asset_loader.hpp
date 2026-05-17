#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include "core/uuid.hpp"

#include <string>
#include <d3d11.h>

class AssetManager;

class AssetLoaderBase {
public:
    virtual ~AssetLoaderBase() = default;
};

template <typename T>
class AssetLoader : public AssetLoaderBase {
protected:
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *deviceContext = nullptr;
    AssetManager *assetManager = nullptr;

public:
    AssetLoader<T>(AssetManager *assetManager) : assetManager(assetManager) {}

    virtual T *Load(AssetID uuid) = 0;

    T *Load(const std::string &path) {
        AssetID uuid = this->assetManager->PathToUUID(path);
        return this->Load(uuid);
    }

    virtual T *CreateDefault() = 0;

    void SetDevice(ID3D11Device *device) { this->device = device; }
    void SetDeviceContext(ID3D11DeviceContext *deviceContext) { this->deviceContext = deviceContext; }
};

#endif