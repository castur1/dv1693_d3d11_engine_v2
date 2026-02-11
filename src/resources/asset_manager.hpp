#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "core/uuid.hpp"

#include <d3d11.h>
#include <string>

class AssetManager {
    ID3D11Device *device;

    std::string assetDir;

public:
    AssetManager();
    ~AssetManager();

    bool Initialize(ID3D11Device *device);

    void MarkAllAssetsUnused();
    void MarkAsUsed(AssetID uuid);
    void CleanUpUnused();

    void SetAssetDirectory(const std::string &path);
};

#endif