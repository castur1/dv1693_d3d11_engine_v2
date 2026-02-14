#include "model_renderer.hpp"

#include "scene/scene.hpp"
#include "core/logging.hpp"

ModelRenderer::ModelRenderer(Entity *owner, bool isActive)
    : Component(owner, isActive), modelID(0) {}

ModelRenderer::~ModelRenderer() {}

void ModelRenderer::OnStart(const Engine_context &context) {
    context.scene->DestroyEntity(this->GetOwner());
}

void ModelRenderer::Update(const Frame_context &context) {}
void ModelRenderer::Render(Renderer *renderer) {}

void ModelRenderer::OnDestroy(const Engine_context &context) {
    LogInfo("Destroyed my parent and thus myself :D\n");
}