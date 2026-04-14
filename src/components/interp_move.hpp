#ifndef INTERP_MOVE_HPP
#define INTERP_MOVE_HPP

#include "scene/component.hpp"

#include <DirectXMath.h>

using namespace DirectX;

enum class Easing_function_type {
    linear = 0,
    inSine,
    outSine,
    inOutSine
};

class InterpMove : public Component {
    float moveTimer  = 0.0f;
    float pauseTimer = 0.0f;

    bool shouldPingPong = false;
    bool isReverse      = false;

    void EaseHelper(float t);
    void EaseLinear();
    void EaseInSine();
    void EaseOutSine();
    void EaseInOutSine();

    void UpdateTimersNormal(float deltaTime);
    void UpdateTimersPingPong(float deltaTime);

public:
    XMFLOAT3 startPosition = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 startRotation = {0.0f, 0.0f, 0.0f}; // Degrees
    XMFLOAT3 startScale    = {1.0f, 1.0f, 1.0f};

    XMFLOAT3 endPosition = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 endRotation = {0.0f, 0.0f, 0.0f}; // Degrees
    XMFLOAT3 endScale    = {1.0f, 1.0f, 1.0f};

    // TODO: Should this work with quaternions instead?
    // It would make rotations smoother, but can't handle rotations >180 degrees
    // To be fair, this component probably needs to be reworked anyways. Keyframes?

    bool shouldIgnorePosition = false;
    bool shouldIgnoreRotation = false;
    bool shouldIgnoreScale    = false;

    float moveDuration  = 1.0f;
    float pauseDuration = 0.0f;

    Easing_function_type easingFunctionType = Easing_function_type::linear;

    InterpMove(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~InterpMove() = default;

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(Renderer *renderer) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(startPosition);
        BIND(startRotation);
        BIND(startScale);

        BIND(endPosition);
        BIND(endRotation);
        BIND(endScale);

        BIND(shouldIgnorePosition);
        BIND(shouldIgnoreRotation);
        BIND(shouldIgnoreScale);

        BIND(moveDuration);
        BIND(pauseDuration);
        BIND(shouldPingPong);

        // TODO: Add enum support to the editor
        int temp = (int)this->easingFunctionType;
        inspector->Field("easingFunctionType", temp);
        this->easingFunctionType = (Easing_function_type)temp;
    }

    void SetShouldPingPong(bool shouldPingPong);
    bool GetShouldPingPong() const;
};

REGISTER_COMPONENT(InterpMove);

#endif