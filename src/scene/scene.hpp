#ifndef SCENE_HPP
#define SCENE_HPP

#include "core/uuid.hpp"
#include "core/frame_context.hpp"

#include <vector>
#include <memory>
#include <unordered_map>

class Renderer;
class Entity;
class SceneManager;

class Scene {
    std::vector<std::unique_ptr<Entity>> entities;
    std::unordered_map<EntityID, Entity *> uuidLookup;

    std::vector<std::unique_ptr<Entity>> entitiesToAdd;
    std::vector<Entity *> entitiesToRemove;

    const Engine_context *context;

    void ResolveEntitiesToAdd();
    void ResolveEntitiesToDestroy();

public:
    Scene();
    ~Scene();

    void Update(const Frame_context &context);
    void Render(Renderer *renderer);

    void Clear();

    Entity *AddEntity(EntityID uuid, bool isActive = true);
    Entity *AddEntity(bool isActive = true);

    void DestroyEntity(Entity *entity);
    void DestroyEntity(EntityID uuid);

    std::vector<Entity *> GetEntities();
    Entity *GetEntityByUUID(EntityID uuid);

    void SetEngineContext(const Engine_context *context);
};

#endif