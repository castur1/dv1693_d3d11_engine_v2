#ifndef RENDER_COMMANDS_HPP
#define RENDER_COMMANDS_HPP

#include "resources/model.hpp"

#include <DirectXMath.h>
#include <d3d11.h>

using namespace DirectX;

struct GeometryCommand {
    ID3D11Buffer *vertexBuffer;
    ID3D11Buffer *indexBuffer;

    UINT indexCount;
    UINT startIndex;
    INT baseVertex;

    AssetHandle<Material> material;

    XMFLOAT4X4 worldMatrix;
};

// Lights, cube map, particle system, ...

#endif