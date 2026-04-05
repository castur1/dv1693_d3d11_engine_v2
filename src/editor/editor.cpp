#include "editor.hpp"
#include "core/logging.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

bool Editor::Initalize(HWND hWnd, ID3D11Device *device, ID3D11DeviceContext *deviceContext) {
    LogInfo("Creating editor...\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();

    float scale = GetDpiForWindow(hWnd) / 96.0f;
    style.ScaleAllSizes(scale);
    io.FontGlobalScale = scale;

    if (!ImGui_ImplWin32_Init(hWnd)) {
        LogError("Failed to initialize Win32 for ImGui");
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, deviceContext)) {
        LogError("Failed to initialize DX11 for ImGui");
        return false;
    }

    return true;
}

void Editor::Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Editor::DrawFPSOverlay(float deltaTime) {
    if (!this->showFPSOverlay)
        return;

    float fps = deltaTime > 1e-6f ? 1.0f / deltaTime : 0.0f;
    this->fpsSmoothed = this->fpsSmoothed * (1.0f - this->fpsAlpha) + fps * this->fpsAlpha;

    const float margin = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImVec2 position = {
        viewport->WorkPos.x + viewport->WorkSize.x - margin,
        viewport->WorkPos.y + margin
    };
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, {1.0f, 0.0f});
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::SetNextWindowSize({0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##fps", nullptr, flags)) {
        ImGui::Text("%-6.1f fps", this->fpsSmoothed);
        ImGui::Text("%.2f ms/f", deltaTime * 1000.0f);
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void Editor::NewFrame(float deltaTime, Scene *scene) {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("FPS overlay", nullptr, &this->showFPSOverlay);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    this->DrawFPSOverlay(deltaTime);
}

void Editor::Render() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool Editor::HasCapturedInput() {
    ImGuiIO &io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

bool Editor::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam) != 0;
}