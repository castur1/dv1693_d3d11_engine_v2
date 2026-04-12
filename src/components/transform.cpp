#include "transform.hpp"
#include "core/logging.hpp"
#include "rendering/renderer.hpp"
#include "scene/entity.hpp"

void Transform::MarkLocalDirty() {
    this->isLocalDirty = true;
    this->MarkWorldDirty();
}

void Transform::MarkWorldDirty() {
    if (this->isWorldDirty)
        return;

    this->isWorldDirty = true;

    Entity *owner = this->GetOwner();
    if (!owner)
        return;

    for (Entity *child : owner->GetChildren()) {
        Transform *childTransform = child->GetComponent<Transform>();
        if (childTransform)
            childTransform->MarkWorldDirty();
    }
}

XMFLOAT3 Transform::ExtractScale(const XMMATRIX &matrix) const {
    XMVECTOR scale, rotation, translation;
    XMMatrixDecompose(&scale, &rotation, &translation, matrix);

    XMFLOAT3 output;
    XMStoreFloat3(&output, scale);
    return output;
}

XMFLOAT4 Transform::ExtractRotation(const XMMATRIX &matrix) const {
    XMVECTOR scale, rotation, translation;
    XMMatrixDecompose(&scale, &rotation, &translation, matrix);

    XMFLOAT4 output;
    XMStoreFloat4(&output, rotation);
    return output;
}

XMFLOAT3 Transform::ExtractTranslation(const XMMATRIX &matrix) const {
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, matrix);
    return {m._41, m._42, m._43};
}

// https://learn.microsoft.com/en-us/windows/win32/api/directxmath/nf-directxmath-xmmatrixrotationrollpitchyaw
// "The order of transformations is roll first, then pitch, and then yaw"
// [ cz * cy + sz * sx * sy,  sz * cx,  sz * sx * cy - cz * sy ]
// [ cz * sx * sy - sz * cy,  cz * cx,  sz * sy + cz * sx * cy ]
// [         cx * sy,          -sx,             cx * cy        ]
// where z = roll, x = pitch, y = yaw and s = sin, c = cos
XMFLOAT3 Transform::QuaternionToEulerAngles(const XMFLOAT4 &quaternion) const {
    XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion));
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, rotationMatrix);

    XMFLOAT3 euler;

    float sinPitch = -m._32; // -(-sx) = sx
    sinPitch = sinPitch < -1.0f ? -1.0f : sinPitch > 1.0f ? 1.0f : sinPitch;

    if (fabsf(sinPitch) < 0.9999f) {
        euler.x = asinf(sinPitch);
        euler.y = atan2f(m._31, m._33); // (cx * sy) / (cx * cy) = sy / cy = ty
        euler.z = atan2f(m._12, m._22); // (sz * cx) / (cz * cx) = sz / cz = tz
    }
    else { // Gimbal lock
        // -(sz * sx * cy - cz * sy) / (cz * cy + sz * sx * sy), sx = 1 => 
        // (cz * sy - sz * cy) / (cz * cy + sz * sy) = s(z - y) / c(z - y) = t(z - y)
        // z = 0 => t(-y) = -ty
        // sx = -1 => t(-y) = -ty

        euler.x = sinPitch > 0.0f ? XM_PIDIV2 : -XM_PIDIV2;
        euler.y = atan2f(-m._13, m._11); 
        euler.z = 0.0f;
    }

    euler.x = XMConvertToDegrees(euler.x);
    euler.y = XMConvertToDegrees(euler.y);
    euler.z = XMConvertToDegrees(euler.z);

    return euler;
}

void Transform::Reflect(ComponentRegistry::Inspector *inspector) {
    inspector->Field("position", this->localPosition);

    XMFLOAT3 euler = this->QuaternionToEulerAngles(this->localRotation);
    inspector->Field("rotation", euler);
    this->SetLocalRotation(euler);

    inspector->Field("scale", this->localScale);
}

void Transform::SetLocalPosition(const XMFLOAT3 &position) {
    this->localPosition = position;
    this->MarkLocalDirty();
}

void Transform::SetLocalPosition(float x, float y, float z) {
    this->SetLocalPosition({x, y, z});
}

XMFLOAT3 Transform::GetLocalPosition() const {
    return this->localPosition;
}

void Transform::SetLocalRotation(const XMFLOAT3 &rotation) {
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(
        XMConvertToRadians(rotation.x),
        XMConvertToRadians(rotation.y),
        XMConvertToRadians(rotation.z)
    );
    XMStoreFloat4(&this->localRotation, quaternion);
    this->MarkLocalDirty();
}

void Transform::SetLocalRotation(float x, float y, float z) {
    this->SetLocalRotation({x, y, z});
}

XMFLOAT3 Transform::GetLocalRotation() const {
    return this->QuaternionToEulerAngles(this->localRotation);
}

void Transform::SetLocalRotationQuaternion(const XMFLOAT4 &quaternion) {
    this->localRotation = quaternion;
    this->MarkLocalDirty();
}

XMFLOAT4 Transform::GetLocalRotationQuaternion() const {
    return this->localRotation;
}

void Transform::SetLocalScale(const XMFLOAT3 &scale) {
    this->localScale = scale;
    this->MarkLocalDirty();
}

void Transform::SetLocalScale(float x, float y, float z) {
    this->SetLocalScale({x, y, z});
}

void Transform::SetLocalScale(float scale) {
    this->SetLocalScale({scale, scale, scale});
}

XMFLOAT3 Transform::GetLocalScale() const {
    return this->localScale;
}

void Transform::SetWorldPosition(const XMFLOAT3 &position) {
    Entity *owner = this->GetOwner();
    Entity *parent = owner ? owner->GetParent() : nullptr;

    if (!parent) {
        this->SetLocalPosition(position);
        return;
    }

    Transform *parentTransform = parent->GetComponent<Transform>();
    if (!parentTransform) {
        this->SetLocalPosition(position);
        return;
    }

    XMMATRIX invParentWorldMatrix = XMMatrixInverse(nullptr, parentTransform->GetWorldMatrix());
    XMVECTOR localPosition = XMVector3Transform(XMLoadFloat3(&position), invParentWorldMatrix);

    XMFLOAT3 result;
    XMStoreFloat3(&result, localPosition);
    this->SetLocalPosition(result);
}

void Transform::SetWorldPosition(float x, float y, float z) {
    this->SetWorldPosition({x, y, z});
}

XMFLOAT3 Transform::GetWorldPosition() const {
    return this->ExtractTranslation(this->GetWorldMatrix());
}

void Transform::SetWorldRotation(const XMFLOAT3 &rotation) {
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(
        XMConvertToRadians(rotation.x),
        XMConvertToRadians(rotation.y),
        XMConvertToRadians(rotation.z)
    );

    XMFLOAT4 output;
    XMStoreFloat4(&output, quaternion);
    this->SetWorldRotationQuaternion(output);
}

void Transform::SetWorldRotation(float x, float y, float z) {
    this->SetWorldRotation({x, y, z});
}

XMFLOAT3 Transform::GetWorldRotation() const {
    XMFLOAT4 quaternion = this->GetWorldRotationQuaternion();
    return this->QuaternionToEulerAngles(quaternion);
}

void Transform::SetWorldRotationQuaternion(const XMFLOAT4 &quaternion) {
    Entity *owner = this->GetOwner();
    Entity *parent = owner ? owner->GetParent() : nullptr;

    if (!parent) {
        this->SetLocalRotationQuaternion(quaternion);
        return;
    }

    Transform *parentTransform = parent->GetComponent<Transform>();
    if (!parentTransform) {
        this->SetLocalRotationQuaternion(quaternion);
        return;
    }
    
    XMFLOAT4 q = parentTransform->GetWorldRotationQuaternion();
    XMVECTOR parentRotation = XMLoadFloat4(&q);
    XMVECTOR invParentRotation = XMQuaternionInverse(parentRotation);
    XMVECTOR localRotation = XMQuaternionMultiply(XMLoadFloat4(&quaternion), invParentRotation);

    XMFLOAT4 result;
    XMStoreFloat4(&result, localRotation);
    this->SetLocalRotationQuaternion(result);
}

XMFLOAT4 Transform::GetWorldRotationQuaternion() const {
    return this->ExtractRotation(this->GetWorldMatrix());
}

void Transform::SetWorldScale(const XMFLOAT3 &scale) {
    Entity *owner = this->GetOwner();
    Entity *parent = owner ? owner->GetParent() : nullptr;

    if (!parent) {
        this->SetLocalScale(scale);
        return;
    }

    Transform *parentTransform = parent->GetComponent<Transform>();
    if (!parentTransform) {
        this->SetLocalScale(scale);
        return;
    }

    XMMATRIX parentWorld = parentTransform->GetWorldMatrix();
    XMVECTOR parentScale, parentRotation, parentTranslation;
    XMMatrixDecompose(&parentScale, &parentRotation, &parentTranslation, parentWorld);

    XMVECTOR localScale = XMVectorDivide(XMLoadFloat3(&scale), parentScale);

    XMFLOAT3 result;
    XMStoreFloat3(&result, localScale);
    this->SetLocalScale(result);
}

void Transform::SetWorldScale(float x, float y, float z) {
    this->SetWorldScale({x, y, z});
}

void Transform::SetWorldScale(float scale) {
    this->SetWorldScale({scale, scale, scale});
}

XMFLOAT3 Transform::GetWorldScale() const {
    return this->ExtractScale(this->GetWorldMatrix());
}

XMMATRIX Transform::GetLocalMatrix() const {
    if (this->isLocalDirty) {
        XMMATRIX scale = XMMatrixScalingFromVector(XMLoadFloat3(&this->localScale));
        XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&this->localRotation));
        XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&this->localPosition));

        XMStoreFloat4x4(&this->cachedLocal, scale * rotation * translation);
        this->isLocalDirty = false;
    }

    return XMLoadFloat4x4(&this->cachedLocal);
}

void Transform::SetWorldMatrix(const XMMATRIX &worldMatrix) {
    XMMATRIX localMatrix = worldMatrix;

    Entity *owner = this->GetOwner();
    if (owner) {
        Entity *parent = owner->GetParent();
        if (parent) {
            Transform *parentTransform = parent->GetComponent<Transform>();
            if (parentTransform) {
                XMMATRIX invParentWorldMatrix = XMMatrixInverse(nullptr, parentTransform->GetWorldMatrix());
                localMatrix = worldMatrix * invParentWorldMatrix;
            }
        }
    }

    XMVECTOR scale, rotation, translation;
    if (!XMMatrixDecompose(&scale, &rotation, &translation, localMatrix))
        return;

    XMStoreFloat3(&this->localScale, scale);
    XMStoreFloat4(&this->localRotation, rotation);
    XMStoreFloat3(&this->localPosition, translation);

    this->MarkLocalDirty();
}

XMMATRIX Transform::GetWorldMatrix() const {
    if (this->isWorldDirty) {
        XMMATRIX local = this->GetLocalMatrix();

        XMMATRIX world = local;

        Entity *owner = this->GetOwner();
        if (owner) {
            Entity *parent = owner->GetParent();
            if (parent) {
                Transform *parentTransform = parent->GetComponent<Transform>();
                if (parentTransform)
                    world = local * parentTransform->GetWorldMatrix();
            }
        }

        XMStoreFloat4x4(&this->cachedWorld, world);
        this->isWorldDirty = false;
    }

    return XMLoadFloat4x4(&this->cachedWorld);
}

XMFLOAT3 Transform::GetForward() const {
    XMFLOAT3 output;
    XMStoreFloat3(&output, this->GetForwardV());
    return output;
}

XMVECTOR Transform::GetForwardV() const {
    XMMATRIX worldMatrix = this->GetWorldMatrix();
    return XMVector3Normalize(worldMatrix.r[2]);
}

XMFLOAT3 Transform::GetRight() const {
    XMFLOAT3 output;
    XMStoreFloat3(&output, this->GetRightV());
    return output;
}

XMVECTOR Transform::GetRightV() const {
    XMMATRIX worldMatrix = this->GetWorldMatrix();
    return XMVector3Normalize(worldMatrix.r[0]);
}

XMFLOAT3 Transform::GetUp() const {
    XMFLOAT3 output;
    XMStoreFloat3(&output, this->GetUpV());
    return output;
}

XMVECTOR Transform::GetUpV() const {
    XMMATRIX worldMatrix = this->GetWorldMatrix();
    return XMVector3Normalize(worldMatrix.r[1]);
}

void Transform::LookAt(const XMFLOAT3 &worldTarget, const XMFLOAT3 &up) {
    XMFLOAT3 worldPosition = this->GetWorldPosition();

    XMVECTOR eye = XMLoadFloat3(&worldPosition);
    XMVECTOR target = XMLoadFloat3(&worldTarget);
    XMVECTOR upVector = XMLoadFloat3(&up);

    if (XMVector3NearEqual(eye, target, XMVectorReplicate(1e-5f)))
        return;

    XMMATRIX viewMatrix = XMMatrixLookAtLH(eye, target, upVector);
    XMMATRIX worldMatrix = XMMatrixInverse(nullptr, viewMatrix);

    XMFLOAT4 rotation = this->ExtractRotation(worldMatrix);
    this->SetWorldRotationQuaternion(rotation);
}

XMFLOAT3 Transform::TransformPoint(const XMFLOAT3 &localPoint) const {
    XMVECTOR worldPoint = XMVector3Transform(XMLoadFloat3(&localPoint), this->GetWorldMatrix());
    XMFLOAT3 output;
    XMStoreFloat3(&output, worldPoint);
    return output;
}

XMFLOAT3 Transform::TransformDirection(const XMFLOAT3 &localDirection) const {
    XMVECTOR worldDir = XMVector3TransformNormal(XMLoadFloat3(&localDirection), this->GetWorldMatrix());
    XMFLOAT3 output;
    XMStoreFloat3(&output, worldDir);
    return output;
}

XMFLOAT3 Transform::InverseTransformPoint(const XMFLOAT3 &worldPoint) const {
    XMMATRIX inverse = XMMatrixInverse(nullptr, this->GetWorldMatrix());
    XMVECTOR localPoint = XMVector3Transform(XMLoadFloat3(&worldPoint), inverse);
    XMFLOAT3 output;
    XMStoreFloat3(&output, localPoint);
    return output;
}

XMFLOAT3 Transform::InverseTransformDirection(const XMFLOAT3 &worldDirection) const {
    XMMATRIX inverse = XMMatrixInverse(nullptr, this->GetWorldMatrix());
    XMVECTOR localDir = XMVector3TransformNormal(XMLoadFloat3(&worldDirection), inverse);
    XMFLOAT3 output;
    XMStoreFloat3(&output, localDir);
    return output;
}
