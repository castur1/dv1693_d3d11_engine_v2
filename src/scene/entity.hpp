#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "core/uuid.hpp"
#include "core/frame_context.hpp"

#include <vector>
#include <memory>

class Renderer;
class Component;
class Scene;

class Entity {
    Scene *scene  = nullptr;

    EntityID uuid = EntityID::invalid;
    bool isActive = true;

    Entity *parent = nullptr;
    std::vector<Entity *> children;

    std::vector<std::unique_ptr<Component>> components;

public:
    bool isStatic = false; // TODO: Should this be public?

    std::string name; // Debugging

    Entity(EntityID uuid, Scene *scene, bool isActive = true);
    Entity(Scene *scene, bool isActive = true);
    ~Entity();

    void OnStart(const Engine_context &context);
    void Update(const Frame_context &context);
    void OnDestroy(const Engine_context &context);

    void SetParent(Entity *newParent);
    Entity *GetParent() const;
    const std::vector<Entity *> &GetChildren() const;

    template<typename T, typename... Args>
    T *AddComponent(Args&&... args) {
        this->components.emplace_back(std::make_unique<T>(this, true, std::forward<Args>(args)...));
        return static_cast<T *>(this->components.back().get());
    }

    Component *AddComponentRaw(Component *component);

    // TODO: RemoveComponent

    // TODO: type_index
    template<typename T>
    T *GetComponent() {
        for (auto &component : this->components) {
            T *ptr = dynamic_cast<T *>(component.get());
            if (ptr) 
                return ptr;
        }

        return nullptr;
    }

    std::vector<Component *> GetComponents();

    void SetActive(bool isActive);
    bool IsActive();

    Scene *GetScene() const;
    EntityID GetID() const;
};

#endif