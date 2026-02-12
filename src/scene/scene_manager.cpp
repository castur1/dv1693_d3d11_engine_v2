#include "scene_manager.hpp"
#include "scene/scene.hpp"
#include "core/logging.hpp"
#include "resources/scene_loader.hpp"

SceneManager::SceneManager() : loader(nullptr) {}

SceneManager::~SceneManager() {
    
}

void SceneManager::ChangeScene(const std::string &name) {
    if (!this->loader->Load(this->currentScene, name)) 
        return;

    this->currentSceneName = name;
}

void SceneManager::RequestSceneChange(const std::string &name) {
    this->targetSceneName = name;
}

bool SceneManager::Initialize(const Engine_context &context) {
    LogInfo("Creating scene manager...\n");

    if (!context.assetManager) {
        LogError("Asset manager was nullptr");
        return false;
    }

    this->loader = std::make_unique<SceneLoader>();
    this->loader->SetAssetManager(context.assetManager);

    this->currentScene.SetEngineContext(&context);

    this->RequestSceneChange("demo_0");

    return true;
}

void SceneManager::Shutdown() {
    this->currentScene.Clear();
}

void SceneManager::Update(const Frame_context &context) {
    if (this->targetSceneName != this->currentSceneName)
        this->ChangeScene(this->targetSceneName);

    this->currentScene.Update(context);
}

void SceneManager::Render(Renderer *renderer) {
    this->currentScene.Render(renderer);
}

Scene * SceneManager::GetCurrentScene() {
    return &this->currentScene;
}