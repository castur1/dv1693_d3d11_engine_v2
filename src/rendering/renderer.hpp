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
    XMFLOAT3   cameraPosition;
    float      pad0;
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
    XMFLOAT3 ambientColour;
    int      directionalLightCount;
    int      spotLightCount;
    float    pad0[3];
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
struct Directional_light_data {
    XMFLOAT3   direction;
    float      intensity;
    XMFLOAT3   colour;
    int        castsShadows;
    int        shadowSliceIndex;
    float      pad0[3];
    XMFLOAT4X4 viewProjectionMatrix;
};
static_assert(sizeof(Directional_light_data) % 16 == 0);

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
    int        shadowSliceIndex;
    float      pad0;
    XMFLOAT4X4 viewProjectionMatrix;
};
static_assert(sizeof(Spot_light_data) % 16 == 0);

enum class Sampler_state_type {
    linearWrap = 0,
    count
};

class Scene;

class Renderer {
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *deviceContext = nullptr;
    IDXGISwapChain *swapChain = nullptr;
    ID3D11RenderTargetView *renderTargetView = nullptr;

    ID3D11SamplerState *samplerStates[(int)Sampler_state_type::count] = {};

    D3D11_VIEWPORT viewport{};
    int width = 0;
    int height = 0;

    float clearColour[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    FrameGraph frameGraph;
    FrameGraph::TextureHandle backbufferHandle = FrameGraph::INVALID_HANDLE;
    FrameGraph::TextureHandle shadowMapDirectionalHandle = FrameGraph::INVALID_HANDLE;
    FrameGraph::TextureHandle shadowMapSpotHandle = FrameGraph::INVALID_HANDLE;

    std::vector<Render_view> views;

    ID3D11VertexShader *shadowVS = nullptr;
    ID3D11InputLayout *shadowLayout = nullptr;
    ID3D11RasterizerState *shadowRS = nullptr;
    ID3D11SamplerState *shadowSampler = nullptr;
    ID3D11VertexShader *gBufferVS = nullptr;
    ID3D11PixelShader *gBufferPS = nullptr;
    ID3D11InputLayout *gBufferLayout = nullptr;
    ID3D11ComputeShader *lightingCS = nullptr;
    ID3D11VertexShader *resolveVS = nullptr;
    ID3D11PixelShader *resolvePS = nullptr;

    static constexpr int MAX_DIRECTIONAL_LIGHTS = 4;
    static constexpr int MAX_SPOT_LIGHTS = 64;

    static constexpr int SHADOW_MAP_DIRECTIONAL_RESOLUTION = 2048;
    static constexpr int SHADOW_MAP_SPOT_RESOLUTION = 1024;

    static constexpr int MAX_DIRECTIONAL_SHADOW_MAPS = 2;
    static constexpr int MAX_SPOT_SHADOW_MAPS = 8;

    ID3D11Texture2D *shadowMapDirectionalTexture = nullptr; // Texture2DArray
    ID3D11ShaderResourceView *shadowMapDirectionalSRV = nullptr;
    ID3D11DepthStencilView *shadowMapDirectionalDSVs[MAX_DIRECTIONAL_SHADOW_MAPS] = {};

    ID3D11Texture2D *shadowMapSpotTexture = nullptr; // Texture2DArray
    ID3D11ShaderResourceView *shadowMapSpotSRV = nullptr;
    ID3D11DepthStencilView *shadowMapSpotDSVs[MAX_SPOT_SHADOW_MAPS] = {};

    ID3D11Buffer *shadowBuffer = nullptr;
    ID3D11Buffer *perObjectBuffer = nullptr;
    ID3D11Buffer *perFrameBuffer = nullptr;
    ID3D11Buffer *perMaterialBuffer = nullptr;
    ID3D11Buffer *lightingBuffer = nullptr;
    ID3D11Buffer *debugResolveBuffer = nullptr; // Debug

    ID3D11Buffer *directionalLightBuffer = nullptr;
    ID3D11ShaderResourceView *directionalLightBufferSRV = nullptr;

    ID3D11Buffer *spotLightBuffer = nullptr;
    ID3D11ShaderResourceView *spotLightBufferSRV = nullptr;

    Per_frame_data currentFrameData{};
    Debug_resolve_data currentDebugData{}; // Debug

    // TODO: Is all this necessary?
    struct Per_frame_shadow_data {
        int directionalCount = 0;
        XMFLOAT4X4 directionalViewProjectionMatrices[MAX_DIRECTIONAL_SHADOW_MAPS] = {};
        int directionalSlotToCommand[MAX_DIRECTIONAL_SHADOW_MAPS] = {};

        int spotCount = 0;
        XMFLOAT4X4 spotViewProjectionMatrices[MAX_SPOT_SHADOW_MAPS] = {};
        int spotSlotToCommand[MAX_SPOT_SHADOW_MAPS] = {};
    };
    Per_frame_shadow_data perFrameShadowData;

    bool CreateInterface(HWND hWnd);
    bool CreateRenderTargetView();
    bool CreateConstantBuffers();
    bool CreateCommonSamplerStates();
    bool LoadDeferredShaders();
    bool CreateShadowResources();
    void SetViewport(int width, int height);

    void BindCommonSamplerStates();

    void UploadPerFrameData(const Render_view &view);
    void UploadLightData(const Render_view &view);

    void BuildFrameGraph();

    void ComputeDirectionalLightMatrices(XMMATRIX &outView, XMMATRIX &outProjection, const Directional_light_command &command, const Render_view &primaryView);
    void ComputeSpotLightMatrices(XMMATRIX &outView, XMMATRIX &outProjection, const Spot_light_command &command);
    void SetupShadowViews(Scene *scene);

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

    Render_view *GetView(View_type type, int index = 0);

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