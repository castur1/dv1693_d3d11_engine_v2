#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <Windows.h>
#include <d3d11.h>

class Scene;
class Entity;

class Editor {
    float fpsSmoothed = 0.0f;
    float fpsAlpha = 0.02f;

    Entity *selectedEntity = nullptr;

    bool showFPSOverlay = true;
    bool showEntityHierarchy = true;
    bool showInspector = true;
    bool showSettings = true;

    void DrawFPSOverlay(float deltaTime);
    void DrawEntityNodeRecursive(Entity *entity);
    void DrawEntityHierarchy(Scene *scene);
    void DrawInspector();
    void DrawSettings();

public:
    Editor() = default;
    ~Editor() = default;

    Editor(const Editor &other) = delete;
    Editor &operator=(const Editor &other) = delete;

    bool Initalize(HWND hWnd, ID3D11Device *device, ID3D11DeviceContext *deviceContext);
    void Shutdown();

    void NewFrame(float deltaTime, Scene *scene);
    void Render();

    bool HasCapturedInput();
    void DisableCursor();

    static bool ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif