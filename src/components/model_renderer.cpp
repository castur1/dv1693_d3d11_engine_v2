#include "model_renderer.hpp"

#include "scene/scene.hpp"
#include "core/logging.hpp"

ModelRenderer::ModelRenderer(Entity *owner, bool isActive)
    : Component(owner, isActive), modelID(0) {}

ModelRenderer::~ModelRenderer() {}

void ModelRenderer::OnStart(const Engine_context &context) {
    this->modelHandle = context.assetManager->GetHandle<Model>(this->modelID);
    Model *model = this->modelHandle.Get();
    for (int i = 0; i < model->subModels.size(); ++i) {
        auto &subModel = model->subModels[i];

        Material *material = subModel.material.Get();
        Texture2D *texture2D = material->diffuseTexture.Get();
        Pipeline_state *pipelineState = material->pipelineState.Get();
    }
}

void ModelRenderer::Update(const Frame_context &context) {}
void ModelRenderer::Render(Renderer *renderer) {}
void ModelRenderer::OnDestroy(const Engine_context &context) {}