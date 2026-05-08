#include "model_renderer.hpp"

#include "core/logging.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"
#include "rendering/render_queue.hpp"
#include "rendering/renderer.hpp"

void ModelRenderer::UpdateWorldBounds() const {
    Model *model = this->modelHandle.Get();
    if (!model) {
        LogWarn("Model was nullptr\n");
        return;
    }

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Transform was nullptr\n");
        return;
    }

    model->localBounds.Transform(this->cachedWorldBounds, transform->GetWorldMatrix());
    this->isWorldBoundsDirty = false;
}

void ModelRenderer::OnStart(const Engine_context &context) {
    this->modelHandle = context.assetManager->GetHandle<Model>(this->modelHandle.GetID());
    this->isWorldBoundsDirty = true;
}

void ModelRenderer::Update(const Frame_context &context) {}

void ModelRenderer::Render(const Render_view &view, RenderQueue &queue) {
    if (!this->isActive)
        return;

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Missing Transform component on owner\n");
        return;
    }

    // TODO: Do culling per sub-model

    const XMMATRIX worldMatrix = XMMatrixTranspose(transform->GetWorldMatrix());

    Model *model = this->modelHandle.Get();
    for (int i = 0; i < model->subModels.size(); ++i) {
        const auto &subModel = model->subModels[i];

        Geometry_command command{};

        command.vertexBuffer = model->vertexBuffer;
        command.indexBuffer = model->indexBuffer;

        command.indexCount = subModel.mesh.indexCount;
        command.startIndex = subModel.mesh.startIndex;
        command.baseVertex = subModel.mesh.baseVertex;

        command.material = subModel.material;
        command.isReflective = this->isReflective;

        XMStoreFloat4x4(&command.worldMatrix, worldMatrix);

        queue.Submit(command);
    }
}

void ModelRenderer::OnDestroy(const Engine_context &context) {}

bool ModelRenderer::GetWorldBounds(BoundingBox &outBounds) const {
    Model *model = this->modelHandle.Get();
    if (!model) {
        LogWarn("Model was nullptr\n");
        return false;
    }

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Transform was nullptr\n");
        return false;
    }

    if (transform->IsWorldDirty())
        this->isWorldBoundsDirty = true;

    if (this->isWorldBoundsDirty)
        this->UpdateWorldBounds();

    outBounds = this->cachedWorldBounds;
    return true;
}