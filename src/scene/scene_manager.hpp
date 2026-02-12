#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "scene/scene.hpp"
#include "core/frame_context.hpp"

#include <string>
#include <memory>

class Renderer;
class SceneLoader;
class AssetManager;

class SceneManager {
    std::unique_ptr<SceneLoader> loader;

    Scene currentScene;
    std::string currentSceneName;
    std::string targetSceneName;

    void ChangeScene(const std::string &name);

public:
    SceneManager();
    ~SceneManager();

    bool Initialize(const Engine_context &context);
    void Shutdown();

    void RequestSceneChange(const std::string &name);

    void Update(const Frame_context &context);
    void Render(Renderer *renderer);

    Scene *GetCurrentScene();
};

#endif