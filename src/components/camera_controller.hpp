#ifndef CAMERA_CONTROLLER_HPP
#define CAMERA_CONTROLLER_HPP

#include "scene/component.hpp"

class CameraController : public Component {
    float fieldOfView = 0.7854f;
    float nearPlane   = 0.1f;
    float farPlane    = 100.0f;

public:
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
    }
};

REGISTER_COMPONENT(CameraController);

#endif