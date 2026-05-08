#ifndef RENDER_COMMANDS_HPP
#define RENDER_COMMANDS_HPP

#include "resources/model.hpp"
#include "resources/asset_manager.hpp"

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
    bool isReflective = false; // TODO: Should probably be part of the material

    XMFLOAT4X4 worldMatrix{};
};

struct Directional_light_command {
    XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
    XMFLOAT3 colour    = {1.0f, 1.0f, 1.0f};
    float    intensity = 0.0f;

    XMFLOAT3 ambientColour = {0.0f, 0.0f, 0.0f};

    bool castsShadows = false;
};

struct Spot_light_command {
    XMFLOAT3 position  = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 direction = {1.0f, 0.0f, 0.0f};
    XMFLOAT3 colour    = {1.0f, 1.0f, 1.0f};
    float    intensity = 0.0f;
    float    range     = 0.0f;

    float innerConeAngle = 0.0f; // Radians
    float outerConeAngle = 0.0f; // Radians

    bool castsShadows = false;
};

struct Skybox_command {
    AssetHandle<Texture_cube> textureCubeHandle{};
};

struct Reflection_probe_command {
    XMFLOAT3 position    = {0.0f, 0.0f, 0.0f};
    float    nearPlane   = 0.1f;
    float    farPlane    = 100.0f;
    float    radius      = 10.0f;
    bool     needsUpdate = true;
};

#endif