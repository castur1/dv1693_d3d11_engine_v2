#ifndef MATERIAL_LOADER_HPP
#define PIPELINE_STATE_LOADER_HPP

#include "resources/asset_loader.hpp"
#include "resources/model.hpp"

class MaterialLoader : public AssetLoader<Material> {
public:
    MaterialLoader(AssetManager *assetManager) : AssetLoader<Material>(assetManager) {}

    Material *Load(const std::string &path) override;
    Material *CreateDefault() override;
};

#endif