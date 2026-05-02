#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "scene/component.hpp"
#include "resources/asset_manager.hpp"
#include "resources/model.hpp"

class Skybox : public Component {
    AssetID textureID = AssetID::invalid;
    AssetHandle<Texture_cube> textureCubeHandle;

public:
    Skybox(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~Skybox() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(textureID);
    }
};

REGISTER_COMPONENT(Skybox);

#endif