#ifndef TEXTURE2D_LOADER_HPP
#define TEXTURE2D_LOADER_HPP

#include "resources/asset_loader.hpp"
#include "resources/model.hpp"

class Texture2DLoader : public AssetLoader<Texture2D> {
public:
    Texture2DLoader(AssetManager *assetManager) : AssetLoader<Texture2D>(assetManager) {}

    Texture2D *Load(const std::string &path) override;
    Texture2D *CreateDefault() override;
};

#endif