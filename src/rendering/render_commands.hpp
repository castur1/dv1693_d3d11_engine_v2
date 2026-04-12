#ifndef RENDER_COMMANDS_HPP
#define RENDER_COMMANDS_HPP

#include "resources/model.hpp"

#include <DirectXMath.h>
#include <d3d11.h>

using namespace DirectX;

struct Geometry_command {
    ID3D11Buffer *vertexBuffer = nullptr;
    ID3D11Buffer *indexBuffer  = nullptr;

    UINT indexCount = 0U;
    UINT startIndex = 0U;
    INT  baseVertex = 0;

    AssetHandle<Material> material{};

    XMFLOAT4X4 worldMatrix{};
};

struct Directional_light_command {
    XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
    XMFLOAT3 colour    = {1.0f, 1.0f, 1.0f};
    float    intensity = 0.0f;

    XMFLOAT3 ambientColour = {0.0f, 0.0f, 0.0f};
};

struct Spot_light_command {
    XMFLOAT3 position  = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 direction = {1.0f, 0.0f, 0.0f};
    XMFLOAT3 colour    = {1.0f, 1.0f, 1.0f};
    float    intensity = 0.0f;
    float    range     = 0.0f;

    // Radians
    float    innerConeAngle = 0.0f;
    float    outerConeAngle = 0.0f;

    bool      castsShadows = false;

    // TODO: viewProjectionMatrix
};

#endif