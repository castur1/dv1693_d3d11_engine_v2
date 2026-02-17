#include "camera_controller.hpp"
#include "core/logging.hpp"
#include "scene/scene.hpp"

CameraController::CameraController(Entity *owner, bool isActive) : Component(owner, isActive) {}
CameraController::~CameraController() {}

void CameraController::OnStart(const Engine_context &context) {
    context.scene->DestroyEntity(this->GetOwner());
}

void CameraController::Update(const Frame_context &context) {}
void CameraController::Render(Renderer *renderer) {}

void CameraController::OnDestroy(const Engine_context &context) {
    LogInfo("Destroyed my parent and thus myself :D\n");
}