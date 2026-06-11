#ifndef SCENE_REGISTRY_HPP
#define SCENE_REGISTRY_HPP

#include <vector>
#include <string>

class SceneRegistry {
    std::vector<std::string> sceneNames;

    std::string sceneDir;

public:
    SceneRegistry() = default;
    ~SceneRegistry() = default;

    void RegisterScenes();
    void Clear() { this->sceneNames.clear(); }

    const std::vector<std::string> &GetSceneNames() const { return this->sceneNames; }

    void SetSceneDirectory(const std::string &sceneDir) { this->sceneDir = sceneDir; }
};

#endif