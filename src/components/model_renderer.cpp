#include "model_renderer.hpp"

#include "scene/scene.hpp"
#include "core/logging.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"
#include "rendering/render_queue.hpp"
#include "rendering/renderer.hpp"

ModelRenderer::ModelRenderer(Entity *owner, bool isActive)
    : Component(owner, isActive), modelID(0) {}

ModelRenderer::~ModelRenderer() {}

void ModelRenderer::OnStart(const Engine_context &context) {
    this->modelHandle = context.assetManager->GetHandle<Model>(this->modelID);
}

void ModelRenderer::Update(const Frame_context &context) {}

void ModelRenderer::Render(Renderer *renderer) {
    if (!this->isActive)
        return;

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Missing Transform component on owner\n");
        return;
    }

    const XMMATRIX worldMatrix = XMMatrixTranspose(transform->GetWorldMatrix());

    RenderQueue &queue = renderer->GetRenderQueue();

    Model *model = this->modelHandle.Get();
    for (int i = 0; i < model->subModels.size(); ++i) {
        const auto &subModel = model->subModels[i];

        GeometryCommand command{};

        command.vertexBuffer = model->vertexBuffer;
        command.indexBuffer = model->indexBuffer;

        command.indexCount = subModel.mesh.indexCount;
        command.startIndex = subModel.mesh.startIndex;
        command.baseVertex = subModel.mesh.baseVertex;

        command.material = subModel.material;

        XMStoreFloat4x4(&command.worldMatrix, worldMatrix);

        queue.Submit(command);
    }
}

void ModelRenderer::OnDestroy(const Engine_context &context) {}