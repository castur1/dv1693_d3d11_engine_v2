#ifndef CAMERA_CONTROLLER_HPP
#define CAMERA_CONTROLLER_HPP

#include "scene/component.hpp"

class CameraController : public Component {
    float fieldOfView;
    float aspectRatio;
    float nearPlane;
    float farPlane;

public:
    CameraController(Entity *owner, bool isActive);
    ~CameraController();

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(Renderer *renderer) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(fieldOfView);
        BIND(aspectRatio);
        BIND(nearPlane);
        BIND(farPlane);
    }
};

REGISTER_COMPONENT(CameraController);

#endif