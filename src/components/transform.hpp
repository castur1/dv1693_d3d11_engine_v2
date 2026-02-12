#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "scene/component.hpp"
#include "core/frame_context.hpp"
#include "scene/component_registry.hpp"

#include <DirectXMath.h>

using namespace DirectX;

class Transform : public Component {
    XMFLOAT3 position;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;

public:
    Transform(Entity *owner, bool isActive, const XMFLOAT3 &position, const XMFLOAT3 &rotation, const XMFLOAT3 &scale);
    Transform(Entity *owner, bool isActive);
    ~Transform();

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(Renderer *renderer) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(position);
        BIND(rotation);
        BIND(scale);
    }

    XMMATRIX GetWorldMatrix() const;

    void SetPosition(const XMFLOAT3 &position);
    void SetRotation(const XMFLOAT3 &rotation);
    void SetScale(const XMFLOAT3 &scale);

    XMFLOAT3 GetPosition() const;
    XMFLOAT3 GetRotation() const;
    XMFLOAT3 GetScale() const;
};

REGISTER_COMPONENT(Transform);

#endif