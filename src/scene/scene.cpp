#include "scene.hpp"
#include "scene/entity.hpp"
#include "rendering/renderer.hpp"

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
    // this->uuidLookup.clear();
    this->entities.clear();
}

//Entity *Scene::AddEntity(EntityID uuid, bool isActive) {
//    this->entities.emplace_back(std::make_unique<Entity>(uuid, this, isActive));
//    return this->uuidLookup[uuid] = this->entities.back().get();
//}

Entity *Scene::AddEntity(bool isActive) {
    //EntityID uuid;
    //return this->AddEntity(uuid, isActive);

    this->entities.emplace_back(std::make_unique<Entity>(this, isActive));
    return this->entities.back().get();
}

std::vector<Entity *> Scene::GetEntities() {
    std::vector<Entity *> result;
    result.reserve(this->entities.size());

    for (auto &entity : this->entities)
        result.push_back(entity.get());

    return result;
}

//Entity *Scene::GetByUUID(EntityID uuid) {
//    auto iter = this->uuidLookup.find(uuid);
//    return iter != this->uuidLookup.end() ? iter->second : nullptr;
//}