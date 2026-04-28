#ifndef SPOT_LIGHT_HPP
#define SPOT_LIGHT_HPP

#include "scene/component.hpp"

#include <DirectXMath.h>

using namespace DirectX;

class SpotLight : public Component {
    XMFLOAT3 colour = {1.0f, 1.0f, 1.0f};
    float intensity = 0.0f;
    float range = 0.0f;

    float innerConeAngle = 30.0f; // Degrees
    float outerConeAngle = 40.0f; // Degrees

    bool castsShadows = false;

public:
    SpotLight(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~SpotLight() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(colour);
        BIND(intensity);
        BIND(range);

        BIND(innerConeAngle);
        BIND(outerConeAngle);

        BIND(castsShadows);
    }
};

REGISTER_COMPONENT(SpotLight);


#endif