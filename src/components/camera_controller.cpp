#include "camera_controller.hpp"

CameraController::CameraController(Entity *owner, bool isActive) : Component(owner, isActive) {}
CameraController::~CameraController() {}

void CameraController::OnStart(const Engine_context &context) {}
void CameraController::Update(const Frame_context &context) {}
void CameraController::Render(Renderer *renderer) {}
void CameraController::OnDestroy(const Engine_context &context) {}