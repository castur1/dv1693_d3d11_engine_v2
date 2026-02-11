#ifndef ENTITY_HPP
#define ENTITY_HPP

// #include "core/uuid.hpp"
#include "core/frame_context.hpp"

#include <vector>
#include <memory>

class Renderer;
class Component;
class Scene;

class Entity {
    // EntityID uuid;
    // TODO: Name for debugging/editor
    Scene *scene;

    std::vector<std::unique_ptr<Component>> components;

public:
    bool isActive;

    // Entity(EntityID uuid, Scene *scene, bool isActive = true);
    Entity(Scene *scene, bool isActive = true);
    ~Entity();

    void OnStart(const Frame_context &context);
    void Update(const Frame_context &context);
    void Render(Renderer *renderer);

    template<typename T, typename... Args>
    T *AddComponent(Args&&... args) {
        this->components.emplace_back(std::make_unique<T>(this, true, std::forward<Args>(args)...));
        return static_cast<T *>(this->components.back().get());
    }

    Component *AddComponentRaw(Component *component);

    template<typename T>
    T *GetComponent() {
        for (auto &component : this->components) {
            T *ptr = dynamic_cast<T *>(component.get());
            if (ptr) return ptr;
        }

        return nullptr;
    }

    std::vector<Component *> GetComponents();

    Scene *GetScene() const;
    // EntityID GetID() const;
};

#endif