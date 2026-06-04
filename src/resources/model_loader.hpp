#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include "resources/asset_loader.hpp"
#include "resources/assets.hpp"

#include <vector>

class ModelLoader : public AssetLoader<Model> {
    void ComputeTangents(std::vector<Vertex> &vertices, const std::vector<UINT> &indices);

public:
    ModelLoader(AssetManager *assetManager) : AssetLoader<Model>(assetManager) {}

    Model *Load(AssetID uuid) override;
    Model *CreateDefault() override;
};

#endif