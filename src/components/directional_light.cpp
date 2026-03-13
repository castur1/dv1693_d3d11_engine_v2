#include "directional_light.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"
#include "core/logging.hpp"
#include "rendering/render_queue.hpp"
#include "rendering/renderer.hpp"

void DirectionalLight::OnStart(const Engine_context &context) {}
void DirectionalLight::Update(const Frame_context &context) {}

void DirectionalLight::Render(Renderer *renderer) {
    if (!this->isActive)
        return;

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Missing Transform component on owner\n");
        return;
    }

    Directional_light_command dlc{};
    dlc.direction     = transform->GetDirectionVector();
    dlc.colour        = this->colour;
    dlc.intensity     = this->intensity;
    dlc.ambientColour = this->ambient;

    renderer->GetRenderQueue().Submit(dlc);
}

void DirectionalLight::OnDestroy(const Engine_context &context) {}