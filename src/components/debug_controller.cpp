#include "debug_controller.hpp"
#include "debugging/debug.hpp"
#include "core/input.hpp"
#include "scene/scene_manager.hpp"
#include "scene/entity.hpp"
#include "components/skybox.hpp"

void DebugController::OnStart(const Engine_context &context) {}

void DebugController::Update(const Frame_context &context) {
    if (!this->isActive)
        return;

    if (Input::IsKeyPressed('1'))
        context.engineContext.sceneManager->RequestSceneChange("demo_0");
    if (Input::IsKeyPressed('2'))
        context.engineContext.sceneManager->RequestSceneChange("demo_1");

    if (Input::IsKeyPressed('C'))
        Debug::SetIntegerSetting("renderer.deferredMode", (Debug::GetIntegerSetting("renderer.deferredMode") + 1) % 6);

    int skyboxIndex = Debug::GetIntegerSetting("skybox.index");
    if (Input::IsKeyPressed('F'))
        ++skyboxIndex;
    Debug::SetIntegerSetting("skybox.index", ((skyboxIndex % 9) + 9) % 9);
    Skybox *skybox = this->GetOwner()->GetComponent<Skybox>();
    if (skybox) {
        skybox->isActive = true;
        switch (skyboxIndex) {
            case 0:
                skybox->textureCubeHandle.SetID(AssetID::FromString("220f-2c7f-91c2-0e6e"));
                break;
            case 1:
                skybox->textureCubeHandle.SetID(AssetID::FromString("88b8-d931-0b5b-d141"));
                break;
            case 2:
                skybox->textureCubeHandle.SetID(AssetID::FromString("cf25-b8d0-ac68-2630"));
                break;
            case 3:
                skybox->textureCubeHandle.SetID(AssetID::FromString("8987-5239-c4b9-e148"));
                break;
            case 4:
                skybox->textureCubeHandle.SetID(AssetID::FromString("b3b6-8117-f6f0-0f9f"));
                break;
            case 5:
                skybox->textureCubeHandle.SetID(AssetID::FromString("269e-0a2c-a060-833b"));
                break;
            case 6:
                skybox->textureCubeHandle.SetID(AssetID::FromString("e262-8483-154f-c662"));
                break;
            case 7:
                skybox->textureCubeHandle.SetID(AssetID::FromString("f094-dfd7-d324-10c3"));
                break;
            case 8:
                skybox->isActive = false;
        }
    }
}

void DebugController::Render(const Render_view &view, RenderQueue &queue) {}
void DebugController::OnDestroy(const Engine_context &context) {}