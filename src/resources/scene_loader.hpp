#ifndef SCENE_LOADER_HPP
#define SCENE_LOADER_HPP

#include <string>

class Scene;
class AssetManager;

class SceneLoader {
    AssetManager *assetManager;

    std::string sceneDir;

public:
    SceneLoader() : assetManager(nullptr), sceneDir("scenes/") {}

    virtual bool Load(Scene &scene, const std::string &name);

    void SetAssetManager(AssetManager *assetManager) { this->assetManager = assetManager; }
    void SetSceneDirectory(const std::string &path) { this->sceneDir = path; }
};

#endif