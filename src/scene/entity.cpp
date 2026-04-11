#include "entity.hpp"
#include "scene/component.hpp"
#include "core/logging.hpp"
#include "scene/scene.hpp"

Entity::Entity(EntityID uuid, Scene *scene, bool isActive) : uuid(uuid), scene(scene), isActive(isActive) {}
Entity::Entity(Scene *scene, bool isActive) : uuid(), scene(scene), isActive(isActive) {}

Entity::~Entity() {}

void Entity::OnStart(const Engine_context &context) {
    for (auto &component : this->components)
        component->OnStart(context);
}

void Entity::Update(const Frame_context &context) {
    for (auto &component : this->components)
        component->Update(context);
}

void Entity::Render(Renderer *renderer) {
    for (auto &component : this->components)
        component->Render(renderer);
}

void Entity::OnDestroy(const Engine_context &context) {
    for (auto &component : this->components)
        component->OnDestroy(context);
}

void Entity::SetParent(Entity *newParent) {
    if (newParent == this->parent)
        return;

    Entity *ancestor = newParent;
    while (ancestor) {
        if (ancestor == this)
            return;

        ancestor = ancestor->parent;
    }

    if (this->parent) {
        std::vector<Entity *> siblings = this->parent->children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }

    this->parent = newParent;

    if (newParent)
        newParent->children.push_back(this);

    if (this->scene)
        this->scene->OnEntityParentChanged(this);
}

Entity *Entity::GetParent() const {
    return this->parent;
}

const std::vector<Entity *> &Entity::GetChildren() const {
    return this->children;
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

void Entity::SetActive(bool isActive) {
    this->isActive = isActive;
}

// Only true if all ancestors are active as well
bool Entity::IsActive() {
    if (!this->parent)
        return this->isActive;

    if (!this->isActive)
        return false;

    return this->parent->IsActive();
}

Scene *Entity::GetScene() const {
    return this->scene;
}

EntityID Entity::GetID() const {
    return this->uuid;
}