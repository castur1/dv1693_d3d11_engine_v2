#include "camera_controller.hpp"
#include "core/logging.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"
#include "rendering/renderer.hpp"

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

    // TODO: Implement movement and stuff

    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

    XMVECTOR right = rotationMatrix.r[0];
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR forward = XMVector3Normalize(XMVector3Cross(right, up));

    XMVECTOR positionVector = XMLoadFloat3(&position);

    forward = rotationMatrix.r[2];
    XMMATRIX viewMatrix = XMMatrixLookToLH(positionVector, forward, up);

    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
        this->fieldOfView, 
        renderer->GetAspectRatio(), 
        this->nearPlane, 
        this->farPlane
    );

    renderer->SetCameraData(viewMatrix, projectionMatrix, position);
}

void CameraController::Render(Renderer *renderer) {}

void CameraController::OnDestroy(const Engine_context &context) {}