#include "scene.hpp"
#include "scene/entity.hpp"
#include "rendering/renderer.hpp"
#include "core/logging.hpp"

#include <algorithm>

Scene::Scene() : context(nullptr) {}
Scene::~Scene() {}

void Scene::Update(const Frame_context &context) {
    this->ResolveEntitiesToAdd();

    for (auto &entity : this->entities)
        entity->Update(context);

    this->ResolveEntitiesToDestroy();
}

void Scene::Render(Renderer *renderer) {
    for (auto &entity : this->entities)
        entity->Render(renderer);
}

void Scene::ResolveEntitiesToAdd() {
    while (!this->entitiesToAdd.empty()) {
        this->entities.push_back(std::move(this->entitiesToAdd.back()));
        this->entitiesToAdd.pop_back();

        Entity *entity = this->entities.back().get();
        this->uuidLookup[entity->GetID()] = entity;
        entity->OnStart(*this->context);
    }
}

void Scene::ResolveEntitiesToDestroy() {
    for (Entity *entity : this->entitiesToRemove) {
        this->uuidLookup.erase(entity->GetID());

        auto iter = std::find_if(
            this->entities.begin(),
            this->entities.end(),
            [&](const std::unique_ptr<Entity> &ptr) { return ptr.get() == entity; }
        );

        if (iter == this->entities.end())
            continue;

        (*iter)->OnDestroy(*this->context);

        if (iter != this->entities.end() - 1)
            std::iter_swap(iter, this->entities.end() - 1);

        this->entities.pop_back();
    }

    this->entitiesToRemove.clear();
}

void Scene::Clear() {
    for (auto &entity : this->entities)
        entity->OnDestroy(*this->context);

    this->entitiesToRemove.clear();
    this->entitiesToAdd.clear();

    this->uuidLookup.clear();
    this->entities.clear();
}

Entity *Scene::AddEntity(EntityID uuid, bool isActive) {
    if (!uuid.IsValid()) {
        LogWarn("Invalid entity UUID");
        return nullptr;
    }

    if (this->uuidLookup.contains(uuid)) {
        LogWarn("Duplicate entity UUID");
        return nullptr;
    }

    this->entitiesToAdd.emplace_back(std::make_unique<Entity>(uuid, this, isActive));
    return this->entitiesToAdd.back().get();
}

Entity *Scene::AddEntity(bool isActive) {
    EntityID uuid;
    return this->AddEntity(uuid, isActive);
}

void Scene::DestroyEntity(Entity *entity) {
    if (entity)
        this->entitiesToRemove.push_back(entity);
}

void Scene::DestroyEntity(EntityID uuid) {
    this->DestroyEntity(this->GetEntityByUUID(uuid));
}

std::vector<Entity *> Scene::GetEntities() {
    std::vector<Entity *> result;
    result.reserve(this->entities.size());

    for (auto &entity : this->entities)
        result.push_back(entity.get());

    return result;
}

Entity *Scene::GetEntityByUUID(EntityID uuid) {
    auto iter = this->uuidLookup.find(uuid);
    return iter != this->uuidLookup.end() ? iter->second : nullptr;
}

void Scene::SetEngineContext(const Engine_context *context) {
    if (this->context) {
        LogWarn("Tried to reassign the scene's engine context\n");
        return;
    }

    if (!context)
        LogError("Engine context was nullptr");

    this->context = context;
}