#include "reflection_probe.hpp"
#include "rendering/render_view.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"

void ReflectionProbe::OnStart(const Engine_context &context) {}
void ReflectionProbe::Update(const Frame_context &context) {}

void ReflectionProbe::Render(const Render_view &view, RenderQueue &queue) {
    if (!this->isActive)
        return;

    if (view.type != View_type::primary)
        return;

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Missing Transform on owner\n");
        return;
    }

    Reflection_probe_command command{};
    command.position = transform->GetWorldPosition();
    command.nearPlane = this->nearPlane;
    command.farPlane = this->farPlane;
    command.radius = this->radius;

    queue.Submit(command);
}

void ReflectionProbe::OnDestroy(const Engine_context &context) {}