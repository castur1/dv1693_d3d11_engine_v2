#ifndef PARTICLE_SYSTEM_HPP
#define PARTICLE_SYSTEM_HPP

#include "rendering/render_data.hpp"
#include "rendering/render_commands.hpp"
#include "rendering/render_view.hpp"
#include "rendering/frame_graph.hpp"
#include "rendering/shared_resources.hpp"

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>

using namespace DirectX;

class Scene;

struct Particle_handles {};

class ParticleSystem {
    friend class Renderer;

    ID3D11ComputeShader *particleCS = nullptr;
    ID3D11VertexShader *particleVS = nullptr;
    ID3D11GeometryShader *particleGS = nullptr;
    ID3D11PixelShader *particlePS = nullptr;
    ID3D11BlendState *additiveBlendState = nullptr;
    ID3D11RasterizerState *particleRS = nullptr;

    ID3D11Buffer *computeBuffer = nullptr;
    ID3D11Buffer *visualBuffer = nullptr;

    void ExecuteParticleRenderPass(
        FrameGraph::ExecutionContext &context,
        const SharedResources &sharedResources,
        const D3D11_VIEWPORT &viewport,
        FrameGraph::TextureHandle depthHandle,
        FrameGraph::TextureHandle lightingOuputHandle
    );

    bool LoadShaders(ID3D11Device *device, const std::string &shaderDir);
    bool CreateBlendState(ID3D11Device *device);
    bool CreateRasterizerState(ID3D11Device *device);
    bool CreateConstantBuffers(ID3D11Device *device);

    ParticleSystem() = default;

    bool Initialize(ID3D11Device *device, const std::string &shaderDir);
    void Shutdown();

public:
    Particle_handles RegisterRenderPasses(
        FrameGraph &frameGraph, 
        const SharedResources &sharedResources, 
        const D3D11_VIEWPORT &viewport, 
        FrameGraph::TextureHandle depthHandle, 
        FrameGraph::TextureHandle lightingOutputHandle, 
        FrameGraph::TextureHandle lightingDummyHandle
    );
};

#endif