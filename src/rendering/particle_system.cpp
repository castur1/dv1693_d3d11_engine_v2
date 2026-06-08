#include "particle_system.hpp"

void ParticleSystem::ExecuteParticleRenderPass(
    FrameGraph::ExecutionContext &context,
    const SharedResources &sharedResources,
    const D3D11_VIEWPORT &viewport,
    FrameGraph::TextureHandle depthHandle,
    FrameGraph::TextureHandle lightingOuputHandle
) {
    ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

    Render_view *view = context.GetView(View_type::primary);
    if (!view || view->queue.particleEmitterCommands.empty())
        return;

    ID3D11RenderTargetView *rtv = context.GetRenderTargetView(lightingOuputHandle);
    ID3D11ShaderResourceView *depthSRV = context.GetShaderResourceView(depthHandle);

    deviceContext->RSSetViewports(1, &viewport);
    deviceContext->RSSetState(this->particleRS);

    deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
    deviceContext->OMSetBlendState(this->additiveBlendState, nullptr, 0xFFFFFFFF);

    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    deviceContext->VSSetShader(this->particleVS, nullptr, 0);
    deviceContext->GSSetShader(this->particleGS, nullptr, 0);
    deviceContext->PSSetShader(this->particlePS, nullptr, 0);

    deviceContext->GSSetConstantBuffers(0, 1, &sharedResources.perFrameBuffer);
    deviceContext->GSSetConstantBuffers(1, 1, &this->visualBuffer);

    deviceContext->PSSetConstantBuffers(0, 1, &this->visualBuffer);
    deviceContext->PSSetShaderResources(1, 1, &depthSRV);

    deviceContext->CSSetShader(this->particleCS, nullptr, 0);
    deviceContext->CSSetConstantBuffers(0, 1, &this->computeBuffer);

    for (const Particle_emitter_command &command : view->queue.particleEmitterCommands) {
        Particle_compute_data computeData{};
        computeData.acceleration     = command.acceleration;
        computeData.deltaTime        = command.deltaTime;
        computeData.maxParticleCount = command.maxParticleCount;

        UploadConstantBuffer(deviceContext, this->computeBuffer, computeData);

        deviceContext->CSSetUnorderedAccessViews(0, 1, &command.unorderedAccessView, nullptr);

        const UINT groups = (command.maxParticleCount + 63) / 64;
        deviceContext->Dispatch(groups, 1, 1);

        ID3D11UnorderedAccessView *nullUAV = nullptr;
        deviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

        Particle_visual_data visualData{};
        visualData.startColour = command.startColour;
        visualData.endColour   = command.endColour;
        visualData.startSize   = command.startSize;
        visualData.endSize     = command.endSize;
        visualData.nearPlane   = view->nearPlane;
        visualData.farPlane    = view->farPlane;

        UploadConstantBuffer(deviceContext, this->visualBuffer, visualData);

        deviceContext->VSSetShaderResources(0, 1, &command.shaderResourceView);

        if (Texture2D *texture = command.textureHandle.Get()) {
            ID3D11ShaderResourceView *srv = texture->shaderResourceView;
            deviceContext->PSSetShaderResources(0, 1, &srv);
        }

        deviceContext->Draw(command.maxParticleCount, 0);
    }

    deviceContext->GSSetShader(nullptr, nullptr, 0);
    deviceContext->RSSetState(nullptr);
    deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    ID3D11RenderTargetView *nullRTV = nullptr;
    deviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);

    ID3D11ShaderResourceView *nullSRVs[2] = {};
    deviceContext->VSSetShaderResources(0, 1, nullSRVs);
    deviceContext->PSSetShaderResources(0, 2, nullSRVs);
}

bool ParticleSystem::LoadShaders(ID3D11Device *device, const std::string &shaderDir) {
    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "cs_particle.cso", bytecode))
        return false;

    HRESULT result = device->CreateComputeShader(bytecode.data(), bytecode.size(), nullptr, &this->particleCS);
    if (FAILED(result)) {
        LogError("Failed to create compute shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "vs_particle.cso", bytecode))
        return false;

    result = device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->particleVS);
    if (FAILED(result)) {
        LogError("Failed to create vertex shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "gs_particle.cso", bytecode))
        return false;

    result = device->CreateGeometryShader(bytecode.data(), bytecode.size(), nullptr, &this->particleGS);
    if (FAILED(result)) {
        LogError("Failed to create geometry shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ps_particle.cso", bytecode))
        return false;

    result = device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &this->particlePS);
    if (FAILED(result)) {
        LogError("Failed to create pixel shader");
        return false;
    }

    return true;
}

bool ParticleSystem::CreateBlendState(ID3D11Device *device) {
    D3D11_BLEND_DESC desc{};
    auto &rt0 = desc.RenderTarget[0];
    rt0.BlendEnable = TRUE;

    rt0.SrcBlend  = D3D11_BLEND_SRC_ALPHA;
    rt0.DestBlend = D3D11_BLEND_ONE;
    rt0.BlendOp   = D3D11_BLEND_OP_ADD;

    rt0.SrcBlendAlpha  = D3D11_BLEND_ZERO;
    rt0.DestBlendAlpha = D3D11_BLEND_ONE;
    rt0.BlendOpAlpha   = D3D11_BLEND_OP_ADD;

    rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT result = device->CreateBlendState(&desc, &this->additiveBlendState);
    if (FAILED(result)) {
        LogError("Failed to create additive blend state");
        return false;
    }

    return true;
}

bool ParticleSystem::CreateRasterizerState(ID3D11Device *device) {
    D3D11_RASTERIZER_DESC desc{};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = TRUE;

    HRESULT result = device->CreateRasterizerState(&desc, &this->particleRS);
    if (FAILED(result)) {
        LogError("Failed to create rasterizer state");
        return false;
    }

    return true;
}

bool ParticleSystem::CreateConstantBuffers(ID3D11Device *device) {
    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    desc.ByteWidth = sizeof(Particle_compute_data);
    HRESULT result = device->CreateBuffer(&desc, nullptr, &this->computeBuffer);
    if (FAILED(result)) {
        LogError("Failed to create particle compute buffer");
        return false;
    }

    desc.ByteWidth = sizeof(Particle_visual_data);
    result = device->CreateBuffer(&desc, nullptr, &this->visualBuffer);
    if (FAILED(result)) {
        LogError("Failed to create particle visual buffer");
        return false;
    }

    return true;
}

bool ParticleSystem::Initialize(ID3D11Device *device, const std::string &shaderDir) {
    LogInfo("Creating particle system...\n");

    if (!this->LoadShaders(device, shaderDir))
        return false;

    if (!this->CreateBlendState(device))
        return false;

    if (!this->CreateRasterizerState(device))
        return false;

    if (!this->CreateConstantBuffers(device))
        return false;

    return true;
}

void ParticleSystem::Shutdown() {
    SafeRelease(this->particleCS);
    SafeRelease(this->particleVS);
    SafeRelease(this->particleGS);
    SafeRelease(this->particlePS);

    SafeRelease(this->additiveBlendState);
    SafeRelease(this->particleRS);

    SafeRelease(this->computeBuffer);
    SafeRelease(this->visualBuffer);
}

Particle_handles ParticleSystem::RegisterRenderPasses(
    FrameGraph &frameGraph,
    const SharedResources &sharedResources,
    const D3D11_VIEWPORT &viewport,
    FrameGraph::TextureHandle depthHandle,
    FrameGraph::TextureHandle lightingOutputHandle,
    FrameGraph::TextureHandle lightingDummyHandle
) {
    Particle_handles handles{};

    struct Particle_pass_data {
        FrameGraph::TextureHandle depth;

        FrameGraph::TextureHandle lightingOutput;
        FrameGraph::TextureHandle dummy;
    };

    // NOTE: This pass also reads from the lightingOutput, but declaring that would likely
    // lead to a cyclic dependency since we also declare a write on it. Instead we rely
    // on a dummy handle to ensure the correct pass ordering.
    // TODO: Add versioning or smth so that this isn't needed.
    frameGraph.AddRenderPass<Particle_pass_data>(
        "Particle pass",
        [&](Particle_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.depth = builder.Read(depthHandle);

            data.lightingOutput = builder.Write(lightingOutputHandle);
            data.dummy = builder.Read(lightingDummyHandle);
        },
        [this, &sharedResources, &viewport](const Particle_pass_data &data, FrameGraph::ExecutionContext &context) {
            this->ExecuteParticleRenderPass(context, sharedResources, viewport, data.depth, data.lightingOutput);
        }
    );

    return handles;
}