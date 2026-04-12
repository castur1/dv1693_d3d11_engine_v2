#include "spot_light.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"
#include "core/logging.hpp"
#include "rendering/render_queue.hpp"
#include "rendering/renderer.hpp"

void SpotLight::OnStart(const Engine_context &context) {}
void SpotLight::Update(const Frame_context &context) {}

void SpotLight::Render(Renderer *renderer) {
    if (!this->isActive)
        return;

    Transform *transform = this->GetOwner()->GetComponent<Transform>();
    if (!transform) {
        LogWarn("Missing Transform component on owner\n");
        return;
    }

    Spot_light_command slc{};
    slc.position       = transform->GetWorldPosition();
    slc.direction      = transform->GetForward();
    slc.colour         = this->colour;
    slc.intensity      = this->intensity;
    slc.range          = this->range;
    slc.innerConeAngle = XMConvertToRadians(this->innerConeAngle);
    slc.outerConeAngle = XMConvertToRadians(this->outerConeAngle);
    slc.castsShadows   = this->castsShadows;

    renderer->GetRenderQueue().Submit(slc);
}

void SpotLight::OnDestroy(const Engine_context &context) {}