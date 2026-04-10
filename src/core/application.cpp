#include "application.hpp"
#include "core/logging.hpp"
#include "scene/component_registry.hpp"
#include "core/input.hpp"

#include <Windows.h>
#include <stdio.h>
#include <chrono>

Application::Application() 
    : context{
        &this->renderer,
        &this->sceneManager, 
        &this->assetManager
    } {}

bool Application::CreateConsole() {
    if (!AllocConsole())
        return false;

    FILE *file;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONIN$", "r", stdin);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return false;

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode))
        return false;

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, mode))
        return false;

    return true;
}

void Application::Update(const Frame_context &context) {
    this->sceneManager.Update(context);
}

void Application::Render(float deltaTime) {
    this->renderer.Begin();
    this->sceneManager.Render(&this->renderer);
    this->renderer.End();

    if (this->isEditorEnabled) {
        this->editor.NewFrame(deltaTime, this->sceneManager.GetCurrentScene());
        this->editor.Render();
    }

    this->renderer.Present();
}

bool Application::Initialize() {
#ifdef _DEBUG
    if (!this->CreateConsole())
        return false;
#endif

    LogInfo("Initializing...\n");
    LogIndent();

    if (!this->window.Initialize(1280, 720, L"D3D11 engine v2 demo"))
        return false;

    if (!this->renderer.Initialize(this->window.GetHandle()))
        return false;

    if (!this->assetManager.Initialize(this->renderer.GetDevice()))
        return false;

    if (!this->sceneManager.Initialize(this->context))
        return false;

    if (!this->editor.Initalize(this->window.GetHandle(), this->renderer.GetDevice(), this->renderer.GetDeviceContext()))
        return false;

    LogUnindent();
    LogInfo("Initialization complete!\n");

    return true;
}

void Application::Run() {
    LogInfo("\n");
    LogInfo("Running...\n");

    auto previousTime = std::chrono::high_resolution_clock::now();

    Input::CaptureMouse(this->window.GetHandle());

    while (!this->window.ShouldClose()) {
        this->window.ProcessMessages();

        if (this->window.WasResized()) {
            this->renderer.Resize(this->window.GetWidth(), this->window.GetHeight());
            this->window.ClearResizeFlag();
        }

        // TODO: Let the editor pause the game! Also frame-by-frame

        if (!this->editor.HasCapturedInput())
            Input::Update();
        else
            Input::Clear();

        if (Input::IsKeyPressed(VK_F1))
            this->isEditorEnabled = !this->isEditorEnabled;

        if (Input::IsKeyPressed(VK_F4))
            this->renderer.ToggleFullscreen();

        if (Input::IsKeyPressed(VK_ESCAPE))
            Input::ReleaseMouse();

        if (!Input::IsMouseCaptured() && Input::IsKeyPressed(VK_LBUTTON)) {
            HWND hWnd = this->window.GetHandle();
            HWND foregroundHWnd = GetForegroundWindow();

            if (hWnd == foregroundHWnd) {
                POINT cursorPos;
                if (GetCursorPos(&cursorPos)) {
                    ScreenToClient(hWnd, &cursorPos);

                    RECT clientRect;
                    GetClientRect(hWnd, &clientRect);

                    if (cursorPos.x >= 0 && cursorPos.x < clientRect.right &&
                        cursorPos.y >= 0 && cursorPos.y < clientRect.bottom) {
                        Input::CaptureMouse(this->window.GetHandle());
                    }
                }
            }
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - previousTime;
        float deltaTime = elapsed.count();
        previousTime = currentTime;

        Frame_context context{
            .deltaTime = deltaTime,
            .engineContext = this->context
        };

        this->Update(context);
        this->Render(deltaTime);
    }

    LogInfo("\n");
    LogInfo("Shutdown...\n");

    this->editor.Shutdown();
    this->sceneManager.Shutdown();

    ComponentRegistry::GetMap().clear();
}
