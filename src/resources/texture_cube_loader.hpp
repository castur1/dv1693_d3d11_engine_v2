#ifndef TEXTURE_CUBE_LOADER_HPP
#define TEXTURE_CUBE_LOADER_HPP

#include "resources/asset_loader.hpp"
#include "resources/model.hpp"

class TextureCubeLoader : public AssetLoader<Texture_cube> {
public:
    TextureCubeLoader(AssetManager *assetManager) : AssetLoader<Texture_cube>(assetManager) {}

    // Constructs a cube map from 6 image files.
    // Expects path ending in "_px.<ext>" (e.g. "skybox_px.png")
    // Automatically loads the remaining 5 faces using the same naming convention:
    // _px, _nx, _py, _ny, _pz, _nz
    Texture_cube *Load(const std::string &path) override;

    Texture_cube *CreateDefault() override;

    Texture_cube *CreateFromBitmaps(UINT32 *pixels[6], UINT resolution);
};

#endif