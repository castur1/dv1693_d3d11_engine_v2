#ifndef SCENE_HPP
#define SCENE_HPP

// #include "core/uuid.hpp"

#include <vector>
#include <memory>
// #include <unordered_map>

class Renderer;
class Entity;
class SceneManager;
struct Frame_context;

class Scene {
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<Entity *> pendingEntities;
    // std::unordered_map<EntityID, Entity *> uuidLookup;

public:
    Scene();
    ~Scene();

    void Update(const Frame_context &context);
    void Render(Renderer *renderer);

    void Clear();

    // Entity *AddEntity(EntityID uuid, bool isActive = true);
    Entity *AddEntity(bool isActive = true);

    std::vector<Entity *> GetEntities();
    // Entity *GetByUUID(EntityID uuid);
};

#endif