#include "skybox.hpp"
#include "rendering/render_view.hpp"
#include "rendering/render_commands.hpp"

void Skybox::OnStart(const Engine_context &context) {
    this->textureCubeHandle = context.assetManager->GetHandle<Texture_cube>(this->textureCubeHandle.GetID());
}

void Skybox::Update(const Frame_context &context) {}

void Skybox::Render(const Render_view &view, RenderQueue &queue) {
    if (!this->isActive)
        return;

    if (view.type != View_type::primary && view.type != View_type::cubeFace)
        return;

    Skybox_command command{};
    command.textureCubeHandle = this->textureCubeHandle;

    queue.Submit(command);
}

void Skybox::OnDestroy(const Engine_context &context) {}