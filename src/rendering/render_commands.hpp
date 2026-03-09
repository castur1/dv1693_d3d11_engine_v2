#ifndef RENDER_COMMANDS_HPP
#define RENDER_COMMANDS_HPP

#include "resources/model.hpp"

#include <DirectXMath.h>
#include <d3d11.h>

using namespace DirectX;

struct GeometryCommand {
    ID3D11Buffer *vertexBuffer = nullptr;
    ID3D11Buffer *indexBuffer  = nullptr;

    UINT indexCount = 0U;
    UINT startIndex = 0U;
    INT  baseVertex = 0;

    AssetHandle<Material> material{};

    XMFLOAT4X4 worldMatrix{};
};

// Lights, cube map, particle system, ...

#endif