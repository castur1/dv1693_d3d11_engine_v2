#ifndef DEBUG_DRAW_HPP
#define DEBUG_DRAW_HPP

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>

#include <vector>

using namespace DirectX;

// Line/Box/Frustum
// Submit to dynamic line list that is cleared each frame (needs Begin()/NewFrame())
// Just vertex position and vertex colour

struct Debug_vertex {
    XMFLOAT3 position;
    XMFLOAT4 colour;
};

class DebugDraw {
    static ID3D11Device *device;

    static ID3D11Buffer *vertexBuffer;
    static ID3D11InputLayout *inputLayout;

    static ID3D11VertexShader *vertexShader;
    static ID3D11PixelShader *pixelShader;

    static ID3D11Buffer *constantBuffer;

    // TODO: Depth stencil state? Rasterizer state?

    static const UINT MAX_VERTEX_COUNT;
    static std::vector<Debug_vertex> vertices;

    static bool LoadShaders(ID3D11Device *device);

public:
    static bool Initialize(ID3D11Device *device);
    static void Shutdown();

    static void Render(ID3D11DeviceContext *deviceContext, const XMFLOAT4X4 &viewProjectionMatrix);

    static void Line(const XMFLOAT3 &v1, const XMFLOAT3 &v2, const XMFLOAT4 &colour = {1.0f, 1.0f, 1.0f, 1.0f});
    static void Box(const BoundingBox &box, const XMFLOAT4 &colour = {1.0f, 1.0f, 1.0f, 1.0f});
    static void Frustum(const BoundingFrustum &frustum, const XMFLOAT4 &colour = {1.0f, 1.0f, 1.0f, 1.0f});

    // Used for DirectXColors.h colours
    static XMFLOAT4 VectorToFloat4(const XMVECTOR &v);
};

#endif