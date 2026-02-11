#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "scene/scene.hpp"
#include "core/frame_context.hpp"

#include <string>

class Renderer;

class SceneManager {
    // SceneLoader loader;

    Scene currentScene;
    std::string currentSceneName;
    std::string targetSceneName;

    void ChangeScene(const std::string &name);

public:
    SceneManager();
    ~SceneManager();

    bool Initialize();

    void RequestSceneChange(const std::string &name);

    void Update(const Frame_context &context);
    void Render(Renderer *renderer);
};

#endif