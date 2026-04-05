#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <Windows.h>
#include <d3d11.h>

class Scene;

class Editor {
    float fpsSmoothed = 0.0f;
    float fpsAlpha    = 0.02f;

    bool showFPSOverlay = true;

    void DrawFPSOverlay(float deltaTime);

public:
    Editor()  = default;
    ~Editor() = default;

    Editor(const Editor &other)            = delete;
    Editor &operator=(const Editor &other) = delete;

    bool Initalize(HWND hWnd, ID3D11Device *device, ID3D11DeviceContext *deviceContext);
    void Shutdown();

    void NewFrame(float deltaTime, Scene *scene);
    void Render();

    bool HasCapturedInput();

    static bool ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif