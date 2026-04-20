#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "rendering/render_queue.hpp"
#include "rendering/frame_graph.hpp"
#include "rendering/render_view.hpp"

#include <Windows.h>
#include <d3d11.h>

using namespace DirectX;

// CBuffer
struct Per_frame_data { // TODO: Rename to "Per_view_data"?
    XMFLOAT4X4 viewMatrix;
    XMFLOAT4X4 projectionMatrix;
    XMFLOAT4X4 viewProjectionMatrix;
    XMFLOAT4X4 invViewProjectionMatrix;
    XMFLOAT3 cameraPosition;
    float pad0;
};
static_assert(sizeof(Per_frame_data) % 16 == 0);

// CBuffer
struct Per_object_data {
    XMFLOAT4X4 worldMatrix;
    XMFLOAT4X4 worldMatrixInvTranspose;
};
static_assert(sizeof(Per_object_data) % 16 == 0);

// CBuffer
struct Per_material_data {
    XMFLOAT3 materialAmbient;
    float    pad0;
    XMFLOAT3 materialDiffuse;
    float    pad1;
    XMFLOAT3 materialSpecular;
    float    materialSpecularExponent;
};
static_assert(sizeof(Per_material_data) % 16 == 0);

// CBuffer
struct Lighting_data {
    XMFLOAT3 directionalLightDirection;
    float    directionalLightIntensity;
    XMFLOAT3 directionalLightColour;
    int      spotLightCount;
    XMFLOAT3 ambientColour;
    float    pad0;
};
static_assert(sizeof(Lighting_data) % 16 == 0);

// CBuffer
struct Debug_resolve_data {
    int   debugMode;
    float nearPlane;
    float farPlane;
    float pad0;
};
static_assert(sizeof(Debug_resolve_data) % 16 == 0);

// Structured buffer element
struct Spot_light_data {
    XMFLOAT3   position;
    float      intensity;
    XMFLOAT3   direction;
    float      range;
    XMFLOAT3   colour;
    float      cosInnerAngle;
    float      cosOuterAngle;
    int        castsShadows; // C++ bool != HLSL bool
    float      pad0[2];
    XMFLOAT4X4 viewProjectionMatrix;
};
static_assert(sizeof(Spot_light_data) % 16 == 0);

enum class Sampler_state_type {
    linearWrap = 0,
    count
};

class Scene;

class Renderer {
    ID3D11Device           *device           = nullptr;
    ID3D11DeviceContext    *deviceContext    = nullptr;
    IDXGISwapChain         *swapChain        = nullptr;
    ID3D11RenderTargetView *renderTargetView = nullptr;

    ID3D11SamplerState *samplerStates[(int)Sampler_state_type::count] = {};

    D3D11_VIEWPORT viewport{};
    int width  = 0;
    int height = 0;

    float clearColour[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    FrameGraph frameGraph;
    FrameGraph::TextureHandle backbufferHandle = FrameGraph::INVALID_HANDLE;

    std::vector<Render_view> views;

    ID3D11VertexShader  *gBufferVS     = nullptr;
    ID3D11PixelShader   *gBufferPS     = nullptr;
    ID3D11InputLayout   *gBufferLayout = nullptr;
    ID3D11ComputeShader *lightingCS    = nullptr;
    ID3D11VertexShader  *resolveVS     = nullptr;
    ID3D11PixelShader   *resolvePS     = nullptr;

    ID3D11Buffer *perObjectBuffer    = nullptr;
    ID3D11Buffer *perFrameBuffer     = nullptr;
    ID3D11Buffer *perMaterialBuffer  = nullptr;
    ID3D11Buffer *lightingBuffer     = nullptr;
    ID3D11Buffer *debugResolveBuffer = nullptr; // Debug

    static constexpr int MAX_SPOT_LIGHTS         = 256;
    ID3D11Buffer *spotLightBuffer                = nullptr;
    ID3D11ShaderResourceView *spotLightBufferSRV = nullptr;

    Per_frame_data currentFrameData{};
    Debug_resolve_data currentDebugData{}; // Debug

    bool CreateInterface(HWND hWnd);
    bool CreateRenderTargetView();
    bool CreateConstantBuffers();
    bool CreateStructuredBuffers();
    bool CreateCommonSamplerStates();
    bool LoadDeferredShaders();
    void SetViewport(int width, int height);

    void BindCommonSamplerStates();

    void UploadPerFrameData(const Render_view &view);
    void UploadLightData(const Render_view &view);

    void BuildFrameGraph();

    template <typename T>
    void UploadConstantBuffer(ID3D11Buffer *buffer, const T &data) {
        if (!buffer) {
            LogWarn("Buffer was nullptr\n");
            return;
        }

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT result = this->deviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(result)) {
            LogWarn("Failed to map resouce\n");
            return;
        }

        *(T *)mapped.pData = data;
        this->deviceContext->Unmap(buffer, 0);
    }

public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer &other) = delete;
    Renderer &operator=(const Renderer &other) = delete;

    bool Initialize(HWND hWnd);
    bool Resize(int width, int height);

    void NewFrame();
    void Render(Scene *scene);
    void Present();

    void AddView(const Render_view &view);
    void AddView(const Render_view &view, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix, const XMFLOAT3 &position);

    // TODO: This is kinda redundant now, at least nearPlane/farPlane since we're getting those from the Render_view
    void SetDebugData(int debugMode, float nearPlane, float farPlane); // Debug.
    int GetDebugMode(); // Debug

    ID3D11Device *GetDevice() const;
    ID3D11DeviceContext *GetDeviceContext() const;
    int GetWidth() const;
    int GetHeight() const;
    float GetAspectRatio() const;

    void ToggleFullscreen();
    bool IsFullscreened();

    void SetClearColour(float r, float g, float b, float a = 1.0f);
    const float *GetClearColour() const;

    std::vector<Render_view> &GetViews();
    FrameGraph &GetFrameGraph();

    FrameGraph::TextureHandle GetBackbufferHandle() const;
};

#endif