#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "core/uuid.hpp"
#include "resources/asset_cache.hpp"
#include "resources/model.hpp"
#include "resources/asset_registry.hpp"

#include <d3d11.h>
#include <string>

class AssetManager {
    ID3D11Device *device;

    AssetRegistry assetRegistry;

    AssetCache<Model> modelCache;
    AssetCache<Texture2D> texture2DCache;
    AssetCache<Material> materialCache;
    AssetCache<Pipeline_state> pipelineStateCache;

    std::string assetDir;

    void CreateDefaultAssets();
    Model *CreateDefaultModel();
    Texture2D *CreateDefaultTexture2D();
    Material *CreateDefaultMaterial();
    Pipeline_state *CreateDefaultPipelineState();

public:
    AssetManager();
    ~AssetManager();

    bool Initialize(ID3D11Device *device);

    bool LoadFileContents(const std::string &path, void **buffer, size_t *size);
    bool LoadShaders(const std::string &vertexShaderPath, const std::string &pixelShaderPath,
        ID3D11VertexShader **vertexShader, ID3D11PixelShader **pixelShader, ID3D11InputLayout **inputLayout);

    void MarkAllAssetsUnused();
    void MarkAsUsed(AssetID uuid);
    void CleanUpUnused();

    void SetAssetDirectory(const std::string &path);
};

#endif