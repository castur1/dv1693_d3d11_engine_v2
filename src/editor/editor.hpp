#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "core/uuid.hpp"
#include "core/frame_context.hpp"

#include <Windows.h>
#include <d3d11.h>

class Scene;
class SceneManager;
class Entity;
class AssetManager;

class Editor {
    float fpsSmoothed = 0.0f;
    float fpsAlpha = 0.02f;

    EntityID selectedEntityID = EntityID::invalid;
    Entity *selectedEntity = nullptr;

    bool showFPSOverlay = true;
    bool showEntityHierarchy = true;
    bool showInspector = true;
    bool showSettings = true;
    
    bool showOpenScenePopup = false;

    void DrawSceneMenu(SceneManager *sceneManager);
    void DrawFPSOverlay(float deltaTime);
    void DrawEntityNodeRecursive(Entity *entity);
    void DrawEntityHierarchy(Scene *scene);
    void DrawInspector(AssetManager *assetManager);
    void DrawSettings();

public:
    Editor() = default;
    ~Editor() = default;

    Editor(const Editor &other) = delete;
    Editor &operator=(const Editor &other) = delete;

    bool Initalize(HWND hWnd, ID3D11Device *device, ID3D11DeviceContext *deviceContext);
    void Shutdown();

    void NewFrame(float deltaTime, const Engine_context &context);
    void Render();

    bool HasCapturedInput();
    void DisableCursor();

    static bool ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif