#include "scene.hpp"
#include "scene/entity.hpp"
#include "rendering/renderer.hpp"
#include "core/logging.hpp"

Scene::Scene() {}

Scene::~Scene() {}

void Scene::Update(const Frame_context &context) {
    for (int i = 0; i < this->pendingEntities.size(); ++i) // Might reallocate, thus indices
        this->pendingEntities[i]->OnStart(context);
    this->pendingEntities.clear();

    for (auto &entity : this->entities)
        entity->Update(context);
}

void Scene::Render(Renderer *renderer) {
    for (auto &entity : this->entities)
        entity->Render(renderer);
}

void Scene::Clear() {
    this->uuidLookup.clear();
    this->pendingEntities.clear();
    this->entities.clear();
}

Entity *Scene::AddEntity(EntityID uuid, bool isActive) {
    if (!uuid.IsValid()) {
        LogWarn("Invalid entity UUID");
        return nullptr;
    }

    this->entities.emplace_back(std::make_unique<Entity>(uuid, this, isActive));

    Entity *entity = this->entities.back().get();

    this->pendingEntities.push_back(entity);

    this->uuidLookup[uuid] = entity;

    return entity;
}

Entity *Scene::AddEntity(bool isActive) {
    EntityID uuid;
    return this->AddEntity(uuid, isActive);
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