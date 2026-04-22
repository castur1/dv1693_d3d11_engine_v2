#include "camera_controller.hpp"
#include "core/logging.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"
#include "rendering/renderer.hpp"
#include "core/input.hpp"
#include "debugging/debug.hpp"

void CameraController::OnStart(const Engine_context &context) {}

void CameraController::Update(const Frame_context &context) {
    if (!this->isActive)
        return;

    Renderer *renderer = context.engineContext.renderer;

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Missing Transform component on owner\n");
        return;
    }

    XMFLOAT3 rotation = transform->GetLocalRotation();

    float &pitch = rotation.x;
    float &yaw   = rotation.y;
    float &roll  = rotation.z;

    if (Input::IsMouseCaptured()) {
        yaw   += this->mouseSensitivity * Input::GetMouseDeltaX();
        pitch += this->mouseSensitivity * Input::GetMouseDeltaY();
    }

    float rotationSpeed = this->rotationSpeed * context.deltaTime;

    if (Input::IsKeyDown(VK_RIGHT))
        yaw += rotationSpeed;
    if (Input::IsKeyDown(VK_LEFT))
        yaw -= rotationSpeed;
    if (Input::IsKeyDown(VK_UP))
        pitch -= rotationSpeed;
    if (Input::IsKeyDown(VK_DOWN))
        pitch += rotationSpeed;

    pitch = XMMax(-90.0f, XMMin(90.0f, pitch));

    transform->SetLocalRotation(rotation);

    XMFLOAT3 position = transform->GetLocalPosition(); // TODO: This could be XMVECTOR

    XMVECTOR right = transform->GetRightV();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR forward = XMVector3Normalize(XMVector3Cross(right, up));

    XMVECTOR positionVector = XMLoadFloat3(&position);
    XMVECTOR velocityVector = XMLoadFloat3(&this->velocity);

    XMVECTOR targetDirection = XMVectorZero();

    if (Input::IsKeyDown('W'))
        targetDirection = XMVectorAdd(targetDirection, forward);
    if (Input::IsKeyDown('S'))
        targetDirection = XMVectorSubtract(targetDirection, forward);
    if (Input::IsKeyDown('D'))
        targetDirection = XMVectorAdd(targetDirection, right);
    if (Input::IsKeyDown('A'))
        targetDirection = XMVectorSubtract(targetDirection, right);
    if (Input::IsKeyDown(VK_SPACE))
        targetDirection = XMVectorAdd(targetDirection, up);
    if (Input::IsKeyDown(VK_LSHIFT))
        targetDirection = XMVectorSubtract(targetDirection, up);

    if (XMVectorGetX(XMVector3LengthSq(targetDirection)) > 1e-6f)
        targetDirection = XMVector3Normalize(targetDirection);

    float maxSpeed = this->moveSpeed;
    if (Input::IsKeyDown('X'))
        maxSpeed *= 5.0f;

    XMVECTOR targetVelocity = XMVectorScale(targetDirection, maxSpeed);
    velocityVector = XMVectorLerp(velocityVector, targetVelocity, this->acceleration * context.deltaTime);

    if (XMVector3Equal(targetDirection, XMVectorZero()))
        velocityVector = XMVectorLerp(velocityVector, XMVectorZero(), this->drag * context.deltaTime);

    positionVector = XMVectorAdd(positionVector, XMVectorScale(velocityVector, context.deltaTime));

    XMStoreFloat3(&position, positionVector);
    XMStoreFloat3(&this->velocity, velocityVector);

    transform->SetLocalPosition(position);

    forward = transform->GetForwardV();
    XMMATRIX viewMatrix = XMMatrixInverse(nullptr, transform->GetWorldMatrix());

    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(this->fieldOfView), 
        renderer->GetAspectRatio(), 
        this->nearPlane, 
        this->farPlane
    );

    Render_view view{};
    view.type  = View_type::primary;
    view.index = 0;

    view.nearPlane = this->nearPlane;
    view.farPlane  = this->farPlane;

    context.engineContext.renderer->AddView(view, viewMatrix, projectionMatrix, transform->GetWorldPosition());

    // Debug //

    int debugMode = renderer->GetDebugMode();
    if (Input::IsKeyPressed('C'))
        debugMode = (debugMode + 1) % 6;

    renderer->SetDebugData(debugMode, this->nearPlane, this->farPlane);

    if (Input::IsKeyPressed('P')) {
        LogInfo("Position: [%.2f, %.2f, %.2f]\n", position.x, position.y, position.z);
        LogInfo("Rotation: [%.2f, %.2f, %.2f]\n", rotation.x, rotation.y, rotation.z);
    }

    Debug::SetStat("position", position);
    Debug::SetStat("rotation", rotation);
}

void CameraController::Render(const Render_view &view, RenderQueue &queue) {}

void CameraController::OnDestroy(const Engine_context &context) {}