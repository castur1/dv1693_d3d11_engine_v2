#include "camera_controller.hpp"
#include "core/logging.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"
#include "rendering/renderer.hpp"
#include "core/input.hpp"

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

    XMFLOAT3 position = transform->GetPosition();
    XMFLOAT3 rotation = transform->GetRotation();

    float &pitch = rotation.x;
    float &yaw = rotation.y;
    float &roll = rotation.z;

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

    pitch = XMMax(-XM_PIDIV2 + 0.001f, XMMin(XM_PIDIV2 - 0.001f, pitch));

    while (yaw >= XM_2PI)
        yaw -= XM_2PI;
    while (yaw < 0.0f)
        yaw += XM_2PI;

    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

    XMVECTOR right = rotationMatrix.r[0];
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR forward = XMVector3Normalize(XMVector3Cross(right, up));

    XMVECTOR positionVector = XMLoadFloat3(&position);

    float moveSpeed = this->moveSpeed * context.deltaTime;
    if (Input::IsKeyDown('X'))
        moveSpeed *= 3.0f;

    if (Input::IsKeyDown('W'))
        positionVector = XMVectorAdd(positionVector, XMVectorScale(forward, moveSpeed));
    if (Input::IsKeyDown('S'))
        positionVector = XMVectorSubtract(positionVector, XMVectorScale(forward, moveSpeed));
    if (Input::IsKeyDown('D'))
        positionVector = XMVectorAdd(positionVector, XMVectorScale(right, moveSpeed));
    if (Input::IsKeyDown('A'))
        positionVector = XMVectorSubtract(positionVector, XMVectorScale(right, moveSpeed));
    if (Input::IsKeyDown(VK_SPACE))
        positionVector = XMVectorAdd(positionVector, XMVectorScale(up, moveSpeed));
    if (Input::IsKeyDown(VK_LSHIFT))
        positionVector = XMVectorSubtract(positionVector, XMVectorScale(up, moveSpeed));

    XMStoreFloat3(&position, positionVector);

    transform->SetPosition(position);
    transform->SetRotation(rotation);

    forward = rotationMatrix.r[2];
    XMMATRIX viewMatrix = XMMatrixLookToLH(positionVector, forward, up);

    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
        this->fieldOfView, 
        renderer->GetAspectRatio(), 
        this->nearPlane, 
        this->farPlane
    );

    renderer->SetCameraData(viewMatrix, projectionMatrix, position);

    // Debug //

    int debugMode = renderer->GetDebugMode();
    if (Input::IsKeyPressed('C'))
        debugMode = (debugMode + 1) % 6;

    renderer->SetDebugData(debugMode, this->nearPlane, this->farPlane);
}

void CameraController::Render(Renderer *renderer) {}

void CameraController::OnDestroy(const Engine_context &context) {}