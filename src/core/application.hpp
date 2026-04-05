#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "core/window.hpp"
#include "rendering/renderer.hpp"
#include "resources/asset_manager.hpp"
#include "scene/scene_manager.hpp"
#include "core/frame_context.hpp"
#include "editor/editor.hpp"

class Application {
    Window       window;
    Renderer     renderer;
    AssetManager assetManager;
    SceneManager sceneManager;
    Editor       editor;

    const Engine_context context;

    bool isEditorEnabled = true;

    bool CreateConsole();

    void Update(const Frame_context &context);
    void Render(float deltaTime);

public:
    Application();
    ~Application() = default;

    Application(const Application &other)            = delete;
    Application &operator=(const Application &other) = delete;

    bool Initialize();
    void Run();
};

#endif