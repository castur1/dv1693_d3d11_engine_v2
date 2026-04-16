#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "scene/component.hpp"
#include "core/frame_context.hpp"
#include "scene/component_registry.hpp"

#include <DirectXMath.h>

using namespace DirectX;

class Transform : public Component {
    XMFLOAT3 localPosition = {0.0f, 0.0f, 0.0f};
    XMFLOAT4 localRotation = {0.0f, 0.0f, 0.0f, 1.0f}; // Quaternion
    XMFLOAT3 localScale    = {1.0f, 1.0f, 1.0f};

    mutable XMFLOAT4X4 cachedLocal{};
    mutable bool isLocalDirty = true;

    mutable XMFLOAT4X4 cachedWorld{};
    mutable bool isWorldDirty = true;

    void MarkLocalDirty();
    void MarkWorldDirty();

    XMFLOAT3 ExtractScale(const XMMATRIX &matrix) const;
    XMFLOAT4 ExtractRotation(const XMMATRIX &matrix) const;
    XMFLOAT3 ExtractTranslation(const XMMATRIX &matrix) const;

    XMFLOAT3 QuaternionToEulerAngles(const XMFLOAT4 &quaternion) const;

public:
    Transform(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~Transform() = default;

    void OnStart(const Engine_context &context) override {}
    void Update(const Frame_context &context) override {}
    void Render(const Render_view &view, RenderQueue &queue) override {}
    void OnDestroy(const Engine_context &context) override {}

    // NOTE: This doesn't use BIND to provide a cleaner interface
    void Reflect(ComponentRegistry::Inspector *inspector) override;

    // TODO: Add XMVECTOR versions

    // TODO: Transition to using degrees everywhere else in the code? Or some Reflect shenanigans

    void SetLocalPosition(const XMFLOAT3 &position);
    void SetLocalPosition(float x, float y, float z);
    XMFLOAT3 GetLocalPosition() const;

    // Euler angles, in degrees
    void SetLocalRotation(const XMFLOAT3 &rotation);
    void SetLocalRotation(float x, float y, float z);
    XMFLOAT3 GetLocalRotation() const;

    void SetLocalRotationQuaternion(const XMFLOAT4 &quaternion);
    XMFLOAT4 GetLocalRotationQuaternion() const;

    void SetLocalScale(const XMFLOAT3 &scale);
    void SetLocalScale(float x, float y, float z);
    void SetLocalScale(float scale);
    XMFLOAT3 GetLocalScale() const;

    void SetWorldPosition(const XMFLOAT3 &position);
    void SetWorldPosition(float x, float y, float z);
    XMFLOAT3 GetWorldPosition() const;

    // Euler angles, in degrees
    void SetWorldRotation(const XMFLOAT3 &rotation);
    void SetWorldRotation(float x, float y, float z);
    XMFLOAT3 GetWorldRotation() const;

    void SetWorldRotationQuaternion(const XMFLOAT4 &quaternion);
    XMFLOAT4 GetWorldRotationQuaternion() const;

    void SetWorldScale(const XMFLOAT3 &scale);
    void SetWorldScale(float x, float y, float z);
    void SetWorldScale(float scale);
    XMFLOAT3 GetWorldScale() const;

    XMMATRIX GetLocalMatrix() const;

    void SetWorldMatrix(const XMMATRIX &worldMatrix);
    XMMATRIX GetWorldMatrix() const;

    XMFLOAT3 GetForward() const;
    XMVECTOR GetForwardV() const;
    XMFLOAT3 GetRight() const;
    XMVECTOR GetRightV() const;
    XMFLOAT3 GetUp() const;
    XMVECTOR GetUpV() const;

    void LookAt(const XMFLOAT3 &worldTarget, const XMFLOAT3 &up = {0.0f, 1.0f, 0.0});

    // Local space -> World space
    XMFLOAT3 TransformPoint(const XMFLOAT3 &localPoint) const;
    XMFLOAT3 TransformDirection(const XMFLOAT3 &localDirection) const;

    // World space -> Local space
    XMFLOAT3 InverseTransformPoint(const XMFLOAT3 &worldPoint) const;
    XMFLOAT3 InverseTransformDirection(const XMFLOAT3 &worldDirection) const;

    bool IsWorldDirty() const { return this->isWorldDirty; }
};

REGISTER_COMPONENT(Transform);

#endif