#ifndef CAMERA_CONTROLLER_HPP
#define CAMERA_CONTROLLER_HPP

#include "scene/component.hpp"

class CameraController : public Component {
    float fieldOfView = 75.0f; // Degrees
    float nearPlane   = 0.1f;
    float farPlane    = 512.0f;

    XMFLOAT3 velocity{};

public:
    float moveSpeed    = 12.0f;
    float acceleration = 12.0f;
    float drag         = 2.0f;

    float rotationSpeed    = 90.0;
    float mouseSensitivity = 0.09f;

    CameraController(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~CameraController() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(Renderer *renderer) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(fieldOfView);
        BIND(nearPlane);
        BIND(farPlane);

        BIND(moveSpeed);
        BIND(acceleration);
        BIND(drag);

        BIND(rotationSpeed);
        BIND(mouseSensitivity);
    }
};

REGISTER_COMPONENT(CameraController);

#endif