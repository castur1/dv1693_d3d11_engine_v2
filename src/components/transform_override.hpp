#ifndef TRANSFORM_OVERRIDE_HPP
#define TRANSFORM_OVERRIDE_HPP

#include "scene/component.hpp"

class TransformOverride : public Component {
public:
    XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 rotation = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};

    bool shouldIgnorePosition = false;
    bool shouldIgnoreRotation = false;
    bool shouldIgnoreScale = false;

    TransformOverride(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~TransformOverride() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(position);
        BIND(rotation);
        BIND(scale);

        BIND(shouldIgnorePosition);
        BIND(shouldIgnoreRotation);
        BIND(shouldIgnoreScale);
    }
};

REGISTER_COMPONENT(TransformOverride);

#endif