#include "application.hpp"
#include "core/logging.hpp"
#include "scene/component_registry.hpp"

#include <Windows.h>
#include <stdio.h>
#include <chrono>

Application::Application() 
    : context{
        &this->sceneManager, 
        &this->assetManager
    } {}

Application::~Application() {}

bool Application::CreateConsole() {
    if (!AllocConsole())
        return false;

    FILE *file;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONIN$", "r", stdin);

    return true;
}

void Application::Update(const Frame_context &context) {
    this->sceneManager.Update(context);
}

void Application::Render() {
    this->renderer.Begin();

    this->sceneManager.Render(&this->renderer);

    this->renderer.End();
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

    LogUnindent();
    LogInfo("Initialization complete!\n");

    return true;
}

void Application::Run() {
    LogInfo("\n");
    LogInfo("Running...\n");

    auto previousTime = std::chrono::high_resolution_clock::now();

    while (!this->window.ShouldClose()) {
        this->window.ProcessMessages();

        if (this->window.WasResized()) {
            this->renderer.Resize(this->window.GetWidth(), this->window.GetHeight());
            this->window.ClearResizeFlag();
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - previousTime;
        previousTime = currentTime;

        Frame_context context{
            .deltaTime = elapsed.count(),
            .engineContext = this->context
        };

        this->Update(context);
        this->Render();
    }

    LogInfo("\n");
    LogInfo("Shutdown...\n");

    this->sceneManager.Shutdown();

    ComponentRegistry::GetMap().clear();
}
