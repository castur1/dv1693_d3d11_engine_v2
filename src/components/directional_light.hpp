#ifndef DIRECTIONAL_LIGHT_HPP
#define DIRECTIONAL_LIGHT_HPP

#include "scene/component.hpp"

#include <DirectXMath.h>

using namespace DirectX;

class DirectionalLight : public Component {
    XMFLOAT3 colour    = {1.0f, 1.0f, 1.0f};
    float    intensity = 0.0f;
    XMFLOAT3 ambient   = {0.0f, 0.0f, 0.0f};

public:
    DirectionalLight(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~DirectionalLight() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(colour);
        BIND(intensity);
        BIND(ambient);
    }
};

REGISTER_COMPONENT(DirectionalLight);


#endif