#ifndef REFLECTION_PROBE_SYSTEM_HPP
#define REFLECTION_PROBE_SYSTEM_HPP

#include "rendering/render_view.hpp"
#include "rendering/frame_graph.hpp"
#include "rendering/shared_resources.hpp"

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

class Scene;
class ShadowSystem;

struct Reflection_probe_handles {
    FrameGraph::TextureHandle reflectionProbes = FrameGraph::INVALID_HANDLE;

    ID3D11ShaderResourceView *reflectionProbeBufferSRV = nullptr;
};

class ReflectionProbeSystem {
    friend class Renderer;

public:
    static constexpr int MAX_REFLECTION_PROBES = 4;
    static constexpr int REFLECTION_PROBE_RESOLUTION = 256;

private:
    ID3D11VertexShader *reflectionVS = nullptr;
    ID3D11PixelShader *reflectionPS = nullptr;
    ID3D11InputLayout *reflectionLayout = nullptr;
    ID3D11ComputeShader *skyboxCS = nullptr;

    ID3D11Texture2D *probeTexture = nullptr;
    ID3D11ShaderResourceView *probeSRV = nullptr;
    ID3D11RenderTargetView *probeRTVs[MAX_REFLECTION_PROBES * 6] = {};
    ID3D11UnorderedAccessView *probeUAVs[MAX_REFLECTION_PROBES * 6] = {};
    ID3D11Texture2D *probeDepthTexture = nullptr;
    ID3D11DepthStencilView *probeDepthDSV = nullptr;

    ID3D11Buffer *probeBuffer = nullptr;
    ID3D11ShaderResourceView *probeBufferSRV = nullptr;

    struct Per_frame_probe_data {
        int count = 0;
        struct Entry {
            XMFLOAT3 position;
            float radius;
        } entries[MAX_REFLECTION_PROBES];
    } perFrameProbeData;

    ShadowSystem *shadowSystem = nullptr; // Needed to get lighting in the reflections

    void ExectuteReflectionRenderPass(
        FrameGraph::ExecutionContext &context, 
        const SharedResources &sharedResources, 
        const float *clearColour
    );

    bool CreateTextureCubeArray(ID3D11Device *device);
    bool CreateDepthTexture(ID3D11Device *device);
    bool LoadShaders(ID3D11Device *device, const std::string &shaderDir);

    ReflectionProbeSystem() = default;

    bool Initialize(ID3D11Device *device, const std::string &shaderDir, ShadowSystem *shadowSystem);
    void Shutdown();

public:
    void PrepareViews(Scene *scene, const Render_view &primaryView, std::vector<Render_view> &outViews);

    Reflection_probe_handles RegisterRenderPasses(
        FrameGraph &frameGraph, 
        const SharedResources &sharedResources, 
        const float *clearColour
    );

    void UploadProbeData(ID3D11DeviceContext *deviceContext) const;

    int GetActiveProbeCount() const { return this->perFrameProbeData.count; };
};

#endif