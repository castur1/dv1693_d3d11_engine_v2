#ifndef REFLECTION_PROBE_HPP
#define REFLECTION_PROBE_HPP

#include "scene/component.hpp"

class ReflectionProbe : public Component {
    float nearPlane = 0.1f;
    float farPlane = 50.0f;
    float radius = 10.0f;

public:
    ReflectionProbe(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~ReflectionProbe() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(nearPlane);
        BIND(farPlane);
        BIND(radius);
    }
};

REGISTER_COMPONENT(ReflectionProbe);

#endif