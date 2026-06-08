#ifndef SHADOW_SYSTEM_HPP
#define SHADOW_SYSTEM_HPP

#include "rendering/render_commands.hpp"
#include "rendering/render_view.hpp"
#include "rendering/frame_graph.hpp"
#include "rendering/shared_resources.hpp"

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

class Scene;

struct Shadow_handles {
    FrameGraph::TextureHandle shadowMapDirectional = FrameGraph::INVALID_HANDLE;
    FrameGraph::TextureHandle shadowMapSpot = FrameGraph::INVALID_HANDLE;

    ID3D11ShaderResourceView *directionalLightBufferSRV = nullptr;
    ID3D11ShaderResourceView *spotLightBufferSRV = nullptr;
};

class ShadowSystem {
    friend class Renderer;

public:
    static constexpr int MAX_DIRECTIONAL_LIGHTS = 4;
    static constexpr int SHADOW_MAP_DIRECTIONAL_RESOLUTION = 2048;
    static constexpr int MAX_DIRECTIONAL_SHADOW_MAPS = 2;

    static constexpr int MAX_SPOT_LIGHTS = 64;
    static constexpr int SHADOW_MAP_SPOT_RESOLUTION = 1024;
    static constexpr int MAX_SPOT_SHADOW_MAPS = 8;

private:
    ID3D11Texture2D *shadowMapDirectionalTexture = nullptr; // Texture2DArray
    ID3D11ShaderResourceView *shadowMapDirectionalSRV = nullptr;
    ID3D11DepthStencilView *shadowMapDirectionalDSVs[MAX_DIRECTIONAL_SHADOW_MAPS] = {};

    ID3D11Texture2D *shadowMapSpotTexture = nullptr; // Texture2DArray
    ID3D11ShaderResourceView *shadowMapSpotSRV = nullptr;
    ID3D11DepthStencilView *shadowMapSpotDSVs[MAX_SPOT_SHADOW_MAPS] = {};

    ID3D11VertexShader *shadowVS = nullptr;
    ID3D11PixelShader *shadowPS = nullptr;
    ID3D11InputLayout *shadowLayout = nullptr;
    ID3D11RasterizerState *shadowRS = nullptr;

    ID3D11Buffer *shadowBuffer = nullptr;

    ID3D11Buffer *directionalLightBuffer = nullptr;
    ID3D11ShaderResourceView *directionalLightBufferSRV = nullptr;

    ID3D11Buffer *spotLightBuffer = nullptr;
    ID3D11ShaderResourceView *spotLightBufferSRV = nullptr;

    // TODO: There has to be some better way to do this
    struct Per_frame_shadow_data {
        int directionalCount = 0;
        XMFLOAT4X4 directionalViewProjectionMatrices[MAX_DIRECTIONAL_SHADOW_MAPS] = {};
        int directionalSlotToCommand[MAX_DIRECTIONAL_SHADOW_MAPS] = {};

        int spotCount = 0;
        XMFLOAT4X4 spotViewProjectionMatrices[MAX_SPOT_SHADOW_MAPS] = {};
        int spotSlotToCommand[MAX_SPOT_SHADOW_MAPS] = {};
    } perFrameShadowData;

    void ComputeDirectionalLightMatrices(
        XMMATRIX &outView, 
        XMMATRIX &outProjection, 
        const Directional_light_command &command, 
        const Render_view &primaryView
    ) const;

    void ComputeSpotLightMatrices(
        XMMATRIX &outView, 
        XMMATRIX &outProjection, 
        const Spot_light_command &command
    ) const;

    void ExecuteShadowPass(
        FrameGraph::ExecutionContext &context,
        const SharedResources &sharedResources,
        FrameGraph::TextureHandle directionalHandle,
        FrameGraph::TextureHandle spotHandle
    );

    bool CreateConstantBuffers(ID3D11Device *device);
    bool LoadShaders(ID3D11Device *device, const std::string &shaderDir);
    bool CreateRasterizerState(ID3D11Device *device);

    ShadowSystem() = default;

    bool Initialize(ID3D11Device *device, const std::string &shaderDir);
    void Shutdown();

public:
    void PrepareViews(Scene *scene, const Render_view &primaryView, std::vector<Render_view> &outViews);
    Shadow_handles RegisterRenderPasses(FrameGraph &frameGraph, const SharedResources &sharedResources);

    void UploadLightData(
        ID3D11DeviceContext *deviceContext, 
        const Render_view &primaryView, 
        const SharedResources &sharedResources, 
        int reflectionProbeCount
    );

    ID3D11ShaderResourceView *GetDirectionalLightBufferSRV() const { return this->directionalLightBufferSRV; }
    ID3D11ShaderResourceView *GetSpotLightBufferSRV() const { return this->spotLightBufferSRV; }
};

#endif