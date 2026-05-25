#include "transform_override.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"

void TransformOverride::OnStart(const Engine_context &context) {}

void TransformOverride::Update(const Frame_context &context) {
    if (!this->isActive)
        return;

    if (this->shouldIgnorePosition && this->shouldIgnoreRotation && this->shouldIgnoreScale)
        return;

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform)
        return;

    XMFLOAT3 parentPosition = transform->GetWorldPosition();
    XMFLOAT4 parentRotation = transform->GetWorldRotationQuaternion();
    XMFLOAT3 parentScale = transform->GetWorldScale();

    if (!this->shouldIgnorePosition)
        parentPosition = this->position;
    
    if (!this->shouldIgnoreRotation)
        parentRotation = Transform::EulerAnglesToQuaternion(this->rotation);

    if (!this->shouldIgnoreScale)
        parentScale = this->scale;

    XMMATRIX scale = XMMatrixScalingFromVector(XMLoadFloat3(&parentScale));
    XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&parentRotation));
    XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&parentPosition));

    transform->SetWorldMatrix(scale * rotation * translation);
}

void TransformOverride::Render(const Render_view &view, RenderQueue &queue) {}
void TransformOverride::OnDestroy(const Engine_context &context) {}