#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "core/window.hpp"
#include "rendering/renderer.hpp"
#include "resources/asset_manager.hpp"
#include "scene/scene_manager.hpp"
#include "core/frame_context.hpp"

class Application {
    Window window;
    Renderer renderer;
    AssetManager assetManager;
    SceneManager sceneManager;

    const Engine_context context;

    bool CreateConsole();

    void Update(const Frame_context &context);
    void Render();

public:
    Application();
    ~Application();

    bool Initialize();
    void Run();
};

#endif