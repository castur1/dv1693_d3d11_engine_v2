#ifndef MODEL_RENDERER_HPP
#define MODEL_RENDERER_HPP

#include "scene/component.hpp"
#include "core/uuid.hpp"
#include "resources/asset_manager.hpp"
#include "resources/model.hpp"

#include <DirectXCollision.h>

class ModelRenderer : public Component {
    AssetHandle<Model> modelHandle;
    bool isReflective = false; // TODO: Hack to get dynamic cube maps to work. Should probably be part of the material instead.

    mutable BoundingBox cachedWorldBounds{};
    mutable bool isWorldBoundsDirty = true;

    void UpdateWorldBounds() const;

public:
    ModelRenderer(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~ModelRenderer() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    bool GetWorldBounds(BoundingBox &outBounds) const override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        AssetID modelID = this->modelHandle.GetID();
        BIND(modelID);
        this->modelHandle.SetID(modelID);

        BIND(isReflective);
    }
};

REGISTER_COMPONENT(ModelRenderer);

#endif