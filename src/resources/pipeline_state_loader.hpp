#ifndef PIPELINE_STATE_LOADER_HPP
#define PIPELINE_STATE_LOADER_HPP

#include "resources/asset_loader.hpp"
#include "resources/model.hpp"

class PipelineStateLoader : public AssetLoader<Pipeline_state> {
    bool LoadShaders(
        const std::string &vertexShaderPath,
        const std::string &pixelShaderPath,
        ID3D11VertexShader **vertexShader,
        ID3D11PixelShader **pixelShader,
        ID3D11InputLayout **inputLayout
    );

public:
    PipelineStateLoader(AssetManager *assetManager) : AssetLoader<Pipeline_state>(assetManager) {}

    Pipeline_state *Load(const std::string &path) override;
    Pipeline_state *CreateDefault() override;
};

#endif