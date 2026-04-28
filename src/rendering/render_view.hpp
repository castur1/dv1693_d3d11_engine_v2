#ifndef RENDER_VIEW_HPP
#define RENDER_VIEW_HPP

#include "render_queue.hpp"

#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace DirectX;

enum class View_type {
    primary,
    shadowMapDirectional,
    shadowMapSpot,
    cubeFace
};

struct Render_view {
    View_type type = View_type::primary;
    int index = 0;

    XMFLOAT4X4 viewMatrix{};
    XMFLOAT4X4 projectionMatrix{};
    XMFLOAT4X4 viewProjectionMatrix{};
    XMFLOAT4X4 invViewProjectionMatrix{};

    XMFLOAT3 cameraPosition{};
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    BoundingFrustum frustum;
    bool skipFrustumCulling; // TODO: Orthographic views don't work with BoundingFrustum; could use OBB instead

    float shadowDistance = 80.0f;

    RenderQueue queue;
};

#endif