#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

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
    ID3D11Device *device;
    AssetManager *assetManager;

public:
    AssetLoader<T>(AssetManager *assetManager) : device(nullptr), assetManager(assetManager) {}

    virtual T *Load(const std::string &path) = 0;
    virtual T *CreateDefault() = 0;

    void SetDevice(ID3D11Device *device) { this->device = device; }
};

#endif