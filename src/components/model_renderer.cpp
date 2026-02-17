#include "model_renderer.hpp"

#include "scene/scene.hpp"
#include "core/logging.hpp"

ModelRenderer::ModelRenderer(Entity *owner, bool isActive)
    : Component(owner, isActive), modelID(0) {}

ModelRenderer::~ModelRenderer() {}

void ModelRenderer::OnStart(const Engine_context &context) {
    this->test_textureHandle = context.assetManager->GetHandle<Texture2D>(this->modelID);
    Texture2D *texture2D = this->test_textureHandle.Get();
    LogInfo("TEST! width: %d, height: %d\n", texture2D->width, texture2D->height);
}

void ModelRenderer::Update(const Frame_context &context) {}
void ModelRenderer::Render(Renderer *renderer) {}
void ModelRenderer::OnDestroy(const Engine_context &context) {}