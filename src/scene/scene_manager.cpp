#include "scene_manager.hpp"
#include "scene/scene.hpp"
#include "core/logging.hpp"

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

void SceneManager::ChangeScene(const std::string &name) {
    LogInfo("Loading scene '%s'...\n", name.c_str());
    LogIndent();

    //if (!this->loader.Load(this->currentScene, name)) {
    //    LogUnindent();
    //    return;
    //}

    this->currentSceneName = name;

    LogUnindent();
}

void SceneManager::RequestSceneChange(const std::string &name) {
    this->targetSceneName = name;
}

bool SceneManager::Initialize() {
    LogInfo("Creating scene manager...\n");

    this->RequestSceneChange("demo_0");

    return true;
}

void SceneManager::Update(const Frame_context &context) {
    if (this->targetSceneName != this->currentSceneName)
        this->ChangeScene(this->targetSceneName);

    this->currentScene.Update(context);
}

void SceneManager::Render(Renderer *renderer) {
    this->currentScene.Render(renderer);
}