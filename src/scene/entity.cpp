#include "entity.hpp"
#include "scene/component.hpp"
#include "core/logging.hpp"

Entity::Entity(EntityID uuid, Scene *scene, bool isActive) : uuid(uuid), scene(scene), isActive(isActive) {}
Entity::Entity(Scene *scene, bool isActive) : uuid(), scene(scene), isActive(isActive) {}

Entity::~Entity() {}

void Entity::OnStart(const Engine_context &context) {
    for (auto &component : this->components)
        component->OnStart(context);
}

void Entity::Update(const Frame_context &context) {
    if (!this->isActive)
        return;

    for (auto &component : this->components)
        component->Update(context);
}

void Entity::Render(Renderer *renderer) {
    if (!this->isActive)
        return;

    for (auto &component : this->components)
        component->Render(renderer);
}

void Entity::OnDestroy(const Engine_context &context) {
    for (auto &component : this->components)
        component->OnDestroy(context);
}

// Ownership transfer; creates unique_ptr
Component *Entity::AddComponentRaw(Component *component) {
    if (!component) {
        LogWarn("Attempted to add null component to entity");
        return nullptr;
    }

    this->components.emplace_back(component);
    return this->components.back().get();
}

std::vector<Component *> Entity::GetComponents() {
    std::vector<Component *> result;
    result.reserve(this->components.size());

    for (auto &component : this->components)
        result.push_back(component.get());

    return result;
}

Scene *Entity::GetScene() const {
    return this->scene;
}

EntityID Entity::GetID() const {
    return this->uuid;
}