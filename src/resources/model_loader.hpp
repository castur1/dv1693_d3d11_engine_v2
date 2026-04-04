#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include "resources/asset_loader.hpp"
#include "resources/model.hpp"

#include <vector>

class ModelLoader : public AssetLoader<Model> {
    void ComputeTangents(std::vector<Vertex> &vertices, const std::vector<UINT> &indices);

public:
    ModelLoader(AssetManager *assetManager) : AssetLoader<Model>(assetManager) {}

    Model *Load(const std::string &path) override;
    Model *CreateDefault() override;
};

#endif