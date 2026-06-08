#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "rendering/render_queue.hpp"
#include "rendering/frame_graph.hpp"
#include "rendering/render_view.hpp"
#include "rendering/render_data.hpp"
#include "rendering/shared_resources.hpp"
#include "rendering/shadow_system.hpp"
#include "rendering/reflection_probe_system.hpp"
#include "rendering/particle_system.hpp"

#include <Windows.h>

class Scene;

class Renderer {
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *deviceContext = nullptr;
    IDXGISwapChain *swapChain = nullptr;
    ID3D11RenderTargetView *renderTargetView = nullptr;

    D3D11_VIEWPORT viewport{};
    int width = 0;
    int height = 0;
    float clearColour[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    FrameGraph frameGraph;
    FrameGraph::TextureHandle backbufferHandle = FrameGraph::INVALID_HANDLE;
    std::vector<Render_view> views;

    SharedResources sharedResources;

    ShadowSystem shadowSystem;
    ReflectionProbeSystem reflectionSystem;
    ParticleSystem particleSystem;

    ID3D11VertexShader *gBufferVS = nullptr;
    ID3D11PixelShader *gBufferPS = nullptr;
    ID3D11InputLayout *gBufferLayout = nullptr;

    ID3D11ComputeShader *lightingCS = nullptr;

    ID3D11VertexShader *resolveVS = nullptr;
    ID3D11PixelShader *resolvePS = nullptr;

    // Debug
    ID3D11Buffer *debugResolveBuffer = nullptr;
    Debug_resolve_data currentDebugData{};

    bool CreateInterface(HWND hWnd);
    bool CreateRenderTargetView();
    bool LoadGBufferShaders();
    bool LoadLightingShader();
    bool LoadResolveShaders();
    bool CreateConstantBuffers();

    void SetViewport(int width, int height);

    void RegisterGeometryPass(
        FrameGraph::TextureHandle albedoHandle, 
        FrameGraph::TextureHandle normalHandle, 
        FrameGraph::TextureHandle specularHandle, 
        FrameGraph::TextureHandle depthHandle
    );

    void RegisterLightingPass(
        FrameGraph::TextureHandle albedoHandle,
        FrameGraph::TextureHandle normalHandle,
        FrameGraph::TextureHandle specularHandle,
        FrameGraph::TextureHandle depthHandle,
        const Shadow_handles &shadowHandles,
        const Reflection_probe_handles &reflectionHandles,
        FrameGraph::TextureHandle lightingOutputHandle,
        FrameGraph::TextureHandle lightingDummyHandle
    );

    void RegisterResolvePass(
        FrameGraph::TextureHandle lightingOutputHandle,
        FrameGraph::TextureHandle albedoHandle, 
        FrameGraph::TextureHandle normalHandle, 
        FrameGraph::TextureHandle specularHandle, 
        FrameGraph::TextureHandle depthHandle
    );

    void BuildFrameGraph();

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
    void AddView(const Render_view &view, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix);

    // TODO: This is kinda redundant now, at least nearPlane/farPlane since we're getting those from the Render_view
    void SetDebugData(int debugMode, float nearPlane, float farPlane); // Debug.
    int GetDebugMode(); // Debug

    ID3D11Device *GetDevice() const { return this->device; }
    ID3D11DeviceContext *GetDeviceContext() const { return this->deviceContext; }
    int GetWidth() const { return this->width; }
    int GetHeight() const { return this->height; }
    float GetAspectRatio() const { return static_cast<float>(this->width) / this->height; }

    void ToggleFullscreen();
    bool IsFullscreened();
    
    void SetClearColour(float r, float g, float b, float a = 1.0f);
    const float *GetClearColour() const;

    std::vector<Render_view> &GetViews() { return this->views; }
    FrameGraph &GetFrameGraph() { return this->frameGraph; }
    FrameGraph::TextureHandle GetBackbufferHandle() const { return this->backbufferHandle; }
};

#endif