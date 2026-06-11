#include "editor.hpp"
#include "core/logging.hpp"
#include "scene/scene.hpp"
#include "scene/entity.hpp"
#include "scene/component.hpp"
#include "editor/imgui_inspector.hpp"
#include "core/input.hpp"
#include "debugging/debug.hpp"
#include "debugging/debug_draw.hpp"
#include "components/transform.hpp"
#include "scene/scene_manager.hpp"

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

void Editor::DrawSceneMenu(SceneManager *sceneManager) {
    if (!sceneManager)
        return;

    if (ImGui::BeginMenu("Scene")) {
        if (ImGui::MenuItem("Open..."))
            this->showOpenScenePopup = true;

        ImGui::BeginDisabled();
        ImGui::MenuItem("Save");
        ImGui::MenuItem("Save as...");
        ImGui::EndDisabled();

        ImGui::EndMenu();
    }

    if (this->showOpenScenePopup)
        ImGui::OpenPopup("Open...");

    ImGui::SetNextWindowSize({250.0f, 0.0f});
    if (ImGui::BeginPopupModal("Open...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const std::vector<std::string> &names = sceneManager->GetRegistry().GetSceneNames();
        int namesCount = names.size();

        float itemHeight = ImGui::GetTextLineHeightWithSpacing();
        float listHeight = itemHeight * (namesCount < 1 ? 1 : namesCount > 10 ? 10 : namesCount) + 5.0f;

        if (ImGui::BeginListBox("##scene_list", {-FLT_MIN, listHeight})) {
            for (const std::string &name : names) {
                if (ImGui::Selectable(name.c_str())) {
                    sceneManager->RequestSceneChange(name);
                    this->showOpenScenePopup = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndListBox();
        }

        if (ImGui::Button("Cancel")) {
            this->showOpenScenePopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Editor::DrawFPSOverlay(float deltaTime) {
    if (!this->showFPSOverlay)
        return;

    float fps = deltaTime > 1e-6f ? 1.0f / deltaTime : 0.0f;
    this->fpsSmoothed = this->fpsSmoothed * (1.0f - this->fpsAlpha) + fps * this->fpsAlpha;

    const float margin = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImVec2 position = {
        viewport->WorkPos.x + margin,
        viewport->WorkPos.y + margin
    };
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, {0.0f, 0.0f});
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::SetNextWindowSize({0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration       |
        ImGuiWindowFlags_NoInputs           |
        ImGuiWindowFlags_NoMove             |
        ImGuiWindowFlags_NoSavedSettings    |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##fps", nullptr, flags)) {
        ImGui::Text("%-6.1f fps", this->fpsSmoothed);
        ImGui::Text("%.2f ms/f", 1000.0f / this->fpsSmoothed);

        for (const auto &[name, value] : Debug::GetCurrentStats())
            ImGui::Text("%s: %s", name.c_str(), value.c_str());
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void Editor::DrawEntityNodeRecursive(Entity *entity) {
    if (!entity)
        return;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    EntityID uuid = entity->GetID();
    ImGui::PushID(uuid);

    bool isSelected = (entity == this->selectedEntity);
    bool hasChildren = !entity->GetChildren().empty();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_FramePadding  |
        ImGuiTreeNodeFlags_DrawLinesFull |
        ImGuiTreeNodeFlags_OpenOnArrow;

    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf;

    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    std::string label = entity->name.empty() ? uuid.ToString() : entity->name;

    bool isInactive = !entity->IsActive();
    if (isInactive)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);


    bool isOpened = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        if (isSelected) {
            this->selectedEntity = nullptr;
            this->selectedEntityID = EntityID::invalid;
        }
        else {
            this->selectedEntity = entity;
            this->selectedEntityID = uuid;
        }
    }

    if (isOpened) {
        for (Entity *child : entity->GetChildren())
            this->DrawEntityNodeRecursive(child);

        ImGui::TreePop();
    }

    if (isInactive)
        ImGui::PopStyleColor();

    ImGui::PopID();
}

void Editor::DrawEntityHierarchy(Scene *scene) {
    if (!this->showEntityHierarchy || !scene)
        return;

    ImGui::SetNextWindowSize({0.0f, 0.0f}, ImGuiCond_Appearing);

    std::string sceneLabel = "Scene \"" + scene->name + "\"";
    if (!ImGui::Begin(sceneLabel.c_str(), &this->showEntityHierarchy, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("##bg", 1, ImGuiTableFlags_RowBg)) {
        const std::vector<Entity *> &roots = scene->GetRootEntities();
        for (Entity *entity : roots)
            this->DrawEntityNodeRecursive(entity);

        ImGui::EndTable();
    }


    ImGui::End();
}

static std::string GetComponentTypeName(Component *component) {
    std::string name = typeid(*component).name();

    for (const char *prefix : {"class ", "struct "}) {
        std::string p = prefix;
        if (name.substr(0, p.size()) == p) {
            name = name.substr(p.size());
            break;
        }
    }

    return name;
}

void Editor::DrawInspector() {
    if (!this->showInspector)
        return;

    ImGui::SetNextWindowSize({-FLT_MIN, 0.0f}, ImGuiCond_Appearing);

    if (!ImGui::Begin("Inspector", &this->showInspector, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    if (!this->selectedEntity) {
        ImGui::TextDisabled("No entity selected.");
        ImGui::End();
        return;
    }

    ImGui::Text("Name: %s", this->selectedEntity->name.c_str());
    ImGui::Text("UUID: %s", this->selectedEntity->GetID().ToString().c_str());
    ImGui::Text("isStatic: %s", this->selectedEntity->isStatic ? "true" : "false");
         
    bool isActive = this->selectedEntity->IsActive();
    ImGui::Checkbox("isActive", &isActive);
    this->selectedEntity->SetActive(isActive);

    ImGui::Separator();

    std::vector<Component *> components = this->selectedEntity->GetComponents();
    ImGuiInspector inspector;

    if (components.empty()) {
        ImGui::TextDisabled("No components.");
        ImGui::End();
        return;
    }

    for (int i = 0; i < components.size(); ++i) {
        Component *component = components[i];
        if (!component)
            return;

        std::string typeName = GetComponentTypeName(component);

        ImGui::PushID(i);

        if (ImGui::CollapsingHeader(typeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            ImGui::Checkbox("isActive##comp", &component->isActive);

            component->Reflect(&inspector);

            ImGui::Unindent();
        }

        ImGui::PopID();
    }

    ImGui::End();
}

static inline float CalculateInputIntWidth(int value) {
    return ImGui::CalcTextSize(std::to_string(value).c_str()).x + 85.0f;
}

void Editor::DrawSettings() {
    if (!this->showSettings)
        return;

    ImGui::SetNextWindowSize({0.0f, 0.0f}, ImGuiCond_Appearing);

    if (!ImGui::Begin("Settings", &this->showSettings, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    for (const auto &[name, value] : Debug::GetCurrentSettings()) {
        bool newValue = value;
        if (ImGui::Checkbox(name.c_str(), &newValue))
            Debug::SetSetting(name, newValue);
    }

    for (const auto &[name, value] : Debug::GetCurrentIntegerSettings()) {
        int newValue = value;
        ImGui::SetNextItemWidth(CalculateInputIntWidth(value));
        if (ImGui::InputInt(name.c_str(), &newValue)) {
            Debug::SetIntegerSetting(name, newValue);
        }
    }

    ImGui::End();
}

void Editor::NewFrame(float deltaTime, SceneManager *sceneManager) {
    if (!sceneManager) {
        LogWarn("Scene manager was nullptr\n");
        return;
    }

    this->selectedEntity = sceneManager->GetCurrentScene()->GetEntityByUUID(this->selectedEntityID);

    // NOTE: There's a bug where the game freezes temporarily when a mouse button is held down,
    // the mouse is captured, and there's an ImGui window over (0, 0)
    ImGuiIO &io = ImGui::GetIO();
    if (Input::IsMouseCaptured())
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {

            ImGui::MenuItem("FPS overlay", nullptr, &this->showFPSOverlay);
            ImGui::MenuItem("Hierarchy", nullptr, &this->showEntityHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &this->showInspector);
            ImGui::MenuItem("Settings", nullptr, &this->showSettings);

            ImGui::EndMenu();
        }

        this->DrawSceneMenu(sceneManager);

        ImGui::EndMainMenuBar();
    }

    this->DrawFPSOverlay(deltaTime);
    this->DrawEntityHierarchy(sceneManager->GetCurrentScene());
    this->DrawInspector();
    this->DrawSettings();

    if (this->selectedEntity) {
        for (Component *component : this->selectedEntity->GetComponents()) {
            BoundingBox bounds;
            if (component->GetWorldBounds(bounds)) {
                DebugDraw::Box(bounds, {1.0f, 1.0f, 0.5, 1.0f});
            }
        }

        Transform *transform = this->selectedEntity->GetComponent<Transform>();
        if (transform) {
            XMFLOAT3 pivot = transform->GetWorldPosition();
            BoundingBox pivotBox(pivot, {0.1f, 0.1f, 0.1f});
            DebugDraw::Box(pivotBox, {1.0f, 0.5f, 0.5, 1.0f});
        }
    }
}

void Editor::Render() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool Editor::HasCapturedInput() {
    ImGuiIO &io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void Editor::DisableCursor() {
    ImGuiIO &io = ImGui::GetIO();

    io.MousePos = {-FLT_MAX, -FLT_MAX};

    for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
        io.MouseDown[i] = false;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

bool Editor::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam) != 0;
}