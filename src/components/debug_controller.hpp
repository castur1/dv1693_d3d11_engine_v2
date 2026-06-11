#ifndef DEBUG_CONTROLLER_HPP
#define DEBUG_CONTROLLER_HPP

#include "scene/component.hpp"

class DebugController : public Component {
public:
    DebugController(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~DebugController() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {}
};

REGISTER_COMPONENT(DebugController);

#endif