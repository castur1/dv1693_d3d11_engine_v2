#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "scene/component_registry.hpp"
#include "core/frame_context.hpp"

#include <DirectXCollision.h>

using namespace DirectX;

class RenderQueue;
struct Render_view;
class Entity;

class Component {
    Entity *owner;

public:
    bool isActive;

    Component(Entity *owner, bool isActive) : owner(owner), isActive(isActive) {}
    virtual ~Component() = default;

    virtual void OnStart(const Engine_context &context) = 0;
    virtual void Update(const Frame_context &context) = 0;
    virtual void Render(const Render_view &view, RenderQueue &queue) = 0;
    virtual void OnDestroy(const Engine_context &context) = 0;

    virtual bool GetWorldBounds(BoundingBox &outBox) const { return false; }

    virtual void Reflect(ComponentRegistry::Inspector *inspector) = 0;

    Entity *GetOwner() const { return this->owner; }
};

#endif