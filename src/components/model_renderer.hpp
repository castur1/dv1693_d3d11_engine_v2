#ifndef MODEL_RENDERER_HPP
#define MODEL_RENDERER_HPP

#include "scene/component.hpp"
#include "core/uuid.hpp"

class ModelRenderer : public Component {
    AssetID modelID;

public:
    ModelRenderer(Entity *owner, bool isActive);
    ~ModelRenderer();

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(Renderer *renderer) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(modelID);
    }
};

REGISTER_COMPONENT(ModelRenderer);

#endif