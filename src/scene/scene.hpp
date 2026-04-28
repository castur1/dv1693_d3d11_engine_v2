#ifndef SCENE_HPP
#define SCENE_HPP

#include "core/uuid.hpp"
#include "core/frame_context.hpp"
#include "scene/scene_culler.hpp"

#include <vector>
#include <memory>
#include <unordered_map>

class Renderer;
class Entity;
class SceneManager;

class Scene {
    std::vector<std::unique_ptr<Entity>> entities;
    std::unordered_map<EntityID, Entity *> uuidLookup;

    std::vector<Entity *> rootEntities;

    std::vector<std::unique_ptr<Entity>> entitiesToAdd;
    std::vector<Entity *> entitiesToRemove;

    const Engine_context *context;

    SceneCuller culler;

    std::vector<Component *> unculledComponents; // TODO: I don't know how I feel about this. Store somewhere else, or rethink?

    void ResolveEntitiesToAdd();
    void ResolveEntitiesToDestroy();

    void UpdateEntityRecursive(Entity *entity, const Frame_context &context);

public:
    std::string name; // Debugging

    Scene();
    ~Scene();

    void Update(const Frame_context &context);

    void GatherVisibility(std::vector<Render_view> &views); // TODO: Don't know how I feel about this name

    void Clear();

    Entity *AddEntity(EntityID uuid, bool isActive = true);
    Entity *AddEntity(bool isActive = true);

    void DestroyEntity(Entity *entity);
    void DestroyEntity(EntityID uuid);

    void OnEntityParentChanged(Entity *entity);

    std::vector<Entity *> GetEntities();
    const std::vector<Entity *> &GetRootEntities() const;
    Entity *GetEntityByUUID(EntityID uuid);

    void SetEngineContext(const Engine_context *context);
};

#endif