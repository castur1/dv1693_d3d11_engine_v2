#ifndef COMPONENT_HPP
#define COMPONENT_HPP

// #include "scene/component_registry.hpp"
#include "core/frame_context.hpp"

class Renderer;
class Entity;

class Component {
    Entity *owner;

public:
    bool isActive;

    Component(Entity *owner, bool isActive) : owner(owner), isActive(isActive) {}
    virtual ~Component() = default;

    virtual void OnStart(const Frame_context &context) = 0;
    virtual void Update(const Frame_context &context) = 0;
    virtual void Render(Renderer *renderer) = 0;

    // virtual void Reflect(ComponentRegistry::Inspector *inspector) = 0;

    Entity *GetOwner() const { return this->owner; }
};

#endif