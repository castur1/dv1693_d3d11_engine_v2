#include "renderer.hpp"
#include "core/logging.hpp"
#include "resources/asset_manager.hpp"
#include "scene/scene.hpp"
#include "debugging/debug_draw.hpp"
#include "rendering/render_utils.hpp"
#include "components/transform.hpp"

#include <vector>

#undef min
#undef max

static const std::string shaderDir = "assets/shaders/";

Renderer::~Renderer() {
    this->frameGraph.Clear();

    this->particleSystem.Shutdown();
    this->reflectionSystem.Shutdown();
    this->shadowSystem.Shutdown();
    this->sharedResources.Shutdown();

    SafeRelease(this->gBufferVS);
    SafeRelease(this->gBufferPS);
    SafeRelease(this->gBufferLayout);

    SafeRelease(this->tessellationVS);
    SafeRelease(this->tessellationHS);
    SafeRelease(this->tessellationDS);
    SafeRelease(this->tessellationBuffer);

    SafeRelease(this->lightingCS);

    SafeRelease(this->resolveVS);
    SafeRelease(this->resolvePS);

    SafeRelease(this->debugResolveBuffer);

    SafeRelease(this->renderTargetView);

    if (this->deviceContext) {
        this->deviceContext->ClearState();
        this->deviceContext->Flush();
        this->deviceContext->Release();
        this->deviceContext = nullptr;
    }

    if (this->swapChain) {
        this->swapChain->SetFullscreenState(false, nullptr);
        this->swapChain->Release();
        this->swapChain = nullptr;
    }

    if (this->device) {
#ifdef _DEBUG
        ID3D11Debug *debugDevice = nullptr;
        if (SUCCEEDED(this->device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debugDevice))) {
            OutputDebugStringW(L"\n[ReportLiveDeviceObjects start]\n");
            debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            OutputDebugStringW(L"[ReportLiveDeviceObjects end]\n\n");

            debugDevice->Release();
        }
#endif

        this->device->Release();
        this->device = nullptr;
    }
}

bool Renderer::CreateInterface(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC swapChainDesc{};

    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // _SRGB
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT flags = 0;
#if _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG; 
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        1,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &this->swapChain,
        &this->device,
        nullptr,
        &this->deviceContext
    );

    if (FAILED(result)) {
        LogError("Failed to create the device or swap chain");
        return false;
    }

    LogInfo("Device and swap chain created\n");

    return true;
}

bool Renderer::CreateRenderTargetView() {
    ID3D11Texture2D *backbuffer{};
    HRESULT result = this->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);

    if (FAILED(result)) {
        LogError("Failed to create the backbuffer");
        return false;
    }

    LogInfo("Backbuffer created\n");

    result = this->device->CreateRenderTargetView(backbuffer, nullptr, &this->renderTargetView);
    backbuffer->Release();

    if (FAILED(result)) {
        LogError("Failed to create the Render Target View");
        return false;
    }

    LogInfo("Render Target View created\n");

    return true;
}

bool Renderer::LoadGBufferShaders() {
    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_gbuffer.cso", bytecode))
        return false;

    HRESULT result = this->device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->gBufferVS);
    if (FAILED(result)) {
        LogError("Failed to create G-buffer vertex shader");
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    result = this->device->CreateInputLayout(layoutDesc, 4, bytecode.data(), bytecode.size(), &this->gBufferLayout);
    if (FAILED(result)) {
        LogError("Failed to create G-buffer input layout");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ps_gbuffer.cso", bytecode))
        return false;

    result = this->device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &this->gBufferPS);
    if (FAILED(result)) {
        LogError("Failed to create G-buffer pixel shader");
        return false;
    }

    LogInfo("G-buffer shaders loaded\n");
    return true;
}

bool Renderer::LoadTessellationShaders() {
    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_gbuffer_tessellation.cso", bytecode))
        return false;

    HRESULT result = this->device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->tessellationVS);
    if (FAILED(result)) {
        LogError("Failed to create tessellation vertex shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "hs_gbuffer_tessellation.cso", bytecode))
        return false;

    result = this->device->CreateHullShader(bytecode.data(), bytecode.size(), nullptr, &this->tessellationHS);
    if (FAILED(result)) {
        LogError("Failed to create tessellation hull shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ds_gbuffer_tessellation.cso", bytecode))
        return false;

    result = this->device->CreateDomainShader(bytecode.data(), bytecode.size(), nullptr, &this->tessellationDS);
    if (FAILED(result)) {
        LogError("Failed to create tessellation domain shader");
        return false;
    }

    LogInfo("Tessellation shaders loaded\n");
    return true;
}

bool Renderer::LoadLightingShader() {
    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "cs_lighting.cso", bytecode))
        return false;

    HRESULT result = this->device->CreateComputeShader(bytecode.data(), bytecode.size(), nullptr, &this->lightingCS);
    if (FAILED(result)) {
        LogError("Failed to create lighting compute shader");
        return false;
    }

    LogInfo("Lighting shaders loaded\n");
    return true;
}

bool Renderer::LoadResolveShaders() {
    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_resolve.cso", bytecode))
        return false;

    HRESULT result = this->device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->resolveVS);
    if (FAILED(result)) {
        LogError("Failed to create resolve vertex shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ps_resolve.cso", bytecode))
        return false;

    result = this->device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &this->resolvePS);
    if (FAILED(result)) {
        LogError("Failed to create resolve pixel shader");
        return false;
    }

    LogInfo("Resolve shaders loaded\n");
    return true;
}

bool Renderer::CreateConstantBuffers() {
    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    desc.ByteWidth = sizeof(Tessellation_data);
    HRESULT result = this->device->CreateBuffer(&desc, nullptr, &this->tessellationBuffer);
    if (FAILED(result)) {
        LogError("Failed to create tessellation constant buffer");
        return false;
    }

    desc.ByteWidth = sizeof(Debug_resolve_data);
    result = this->device->CreateBuffer(&desc, nullptr, &this->debugResolveBuffer);
    if (FAILED(result)) {
        LogError("Failed to create debug resolve constant buffer");
        return false;
    }

    LogInfo("Constant buffers created\n");
    return true;
}

void Renderer::SetViewport(int width, int height) {
    this->viewport.TopLeftX = 0.0f;
    this->viewport.TopLeftY = 0.0f;
    this->viewport.Width    = static_cast<FLOAT>(width);
    this->viewport.Height   = static_cast<FLOAT>(height);
    this->viewport.MinDepth = 0.0f;
    this->viewport.MaxDepth = 1.0f;
}

void Renderer::RegisterGeometryPass(
    FrameGraph::TextureHandle albedoHandle,
    FrameGraph::TextureHandle normalHandle,
    FrameGraph::TextureHandle specularHandle,
    FrameGraph::TextureHandle depthHandle
) {
    struct Geometry_pass_data {
        FrameGraph::TextureHandle albedo;
        FrameGraph::TextureHandle normal;
        FrameGraph::TextureHandle specular;
        FrameGraph::TextureHandle depth;
    };

    this->frameGraph.AddRenderPass<Geometry_pass_data>(
        "Geometry pass",
        [&](Geometry_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.albedo   = builder.Write(albedoHandle);
            data.normal   = builder.Write(normalHandle);
            data.specular = builder.Write(specularHandle);
            data.depth    = builder.Write(depthHandle);
        },
        [this](const Geometry_pass_data &data, FrameGraph::ExecutionContext &context) {
            ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

            Render_view *view = context.GetView(View_type::primary);
            if (!view) {
                LogWarn("View was nullptr\n");
                return;
            }

            this->sharedResources.UploadPerFrameData(deviceContext, *view);

            ID3D11RenderTargetView *rtvs[3] = {
                context.GetRenderTargetView(data.albedo),
                context.GetRenderTargetView(data.normal),
                context.GetRenderTargetView(data.specular)
            };

            ID3D11DepthStencilView *dsv = context.GetDepthStencilView(data.depth);

            const float clearAlbedo[4]   = {0.0f, 0.0f, 0.0f, 0.0f};
            const float clearNormal[4]   = {0.5f, 0.5f, 1.0f, 0.0f};
            const float clearSpecular[4] = {0.0f, 0.0f, 0.0f, 0.0f};

            deviceContext->ClearRenderTargetView(rtvs[0], clearAlbedo);
            deviceContext->ClearRenderTargetView(rtvs[1], clearNormal);
            deviceContext->ClearRenderTargetView(rtvs[2], clearSpecular);
            deviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

            deviceContext->OMSetRenderTargets(3, rtvs, dsv);

            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            deviceContext->IASetInputLayout(this->gBufferLayout);

            deviceContext->VSSetShader(this->gBufferVS, nullptr, 0);
            deviceContext->PSSetShader(this->gBufferPS, nullptr, 0);

            deviceContext->VSSetConstantBuffers(0, 1, &this->sharedResources.perFrameBuffer);
            deviceContext->VSSetConstantBuffers(1, 1, &this->sharedResources.perObjectBuffer);
            deviceContext->PSSetConstantBuffers(2, 1, &this->sharedResources.perMaterialBuffer);

            deviceContext->RSSetViewports(1, &this->viewport);

            for (Geometry_command &command : view->queue.geometryCommands) {
                Per_object_data perObjectData{};
                perObjectData.worldMatrix = command.worldMatrix;
                XMStoreFloat4x4(
                    &perObjectData.worldMatrixInvTranspose, 
                    XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&command.worldMatrix)))
                );

                UploadConstantBuffer(deviceContext, this->sharedResources.perObjectBuffer, perObjectData);

                Material *material = command.material.Get();
                if (material) {
                    Per_material_data perMaterialData{};
                    perMaterialData.materialDiffuse          = material->diffuseColour;
                    perMaterialData.isReflective             = command.isReflective;
                    perMaterialData.materialSpecular         = material->specularColour;
                    perMaterialData.materialSpecularExponent = material->specularExponent;

                    UploadConstantBuffer(deviceContext, this->sharedResources.perMaterialBuffer, perMaterialData);

                    ID3D11ShaderResourceView *srvs[2] = {
                        material->diffuseTexture.Get()->shaderResourceView,
                        material->normalTexture.Get()->shaderResourceView
                    };
                    deviceContext->PSSetShaderResources(0, 2, srvs);
                }

                const UINT stride = sizeof(Vertex);
                const UINT offset = 0;
                deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
            }

            if (!view->queue.tessellatedGeometryCommands.empty()) {
                deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

                deviceContext->VSSetShader(this->tessellationVS, nullptr, 0);
                deviceContext->HSSetShader(this->tessellationHS, nullptr, 0);
                deviceContext->DSSetShader(this->tessellationDS, nullptr, 0);

                deviceContext->HSSetConstantBuffers(0, 1, &this->sharedResources.perFrameBuffer);
                deviceContext->HSSetConstantBuffers(3, 1, &this->tessellationBuffer);

                deviceContext->DSSetConstantBuffers(0, 1, &this->sharedResources.perFrameBuffer);
                deviceContext->DSSetConstantBuffers(3, 1, &this->tessellationBuffer);

                for (Geometry_command &command : view->queue.tessellatedGeometryCommands) {
                    Per_object_data perObjectData{};
                    perObjectData.worldMatrix = command.worldMatrix;
                    XMStoreFloat4x4(
                        &perObjectData.worldMatrixInvTranspose, 
                        XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&command.worldMatrix)))
                    );

                    UploadConstantBuffer(deviceContext, this->sharedResources.perObjectBuffer, perObjectData);

                    Material *material = command.material.Get();
                    if (material) {
                        Per_material_data perMaterialData{};
                        perMaterialData.materialDiffuse          = material->diffuseColour;
                        perMaterialData.isReflective             = command.isReflective;
                        perMaterialData.materialSpecular         = material->specularColour;
                        perMaterialData.materialSpecularExponent = material->specularExponent;

                        UploadConstantBuffer(deviceContext, this->sharedResources.perMaterialBuffer, perMaterialData);

                        XMFLOAT3 scale = Transform::ExtractScale(XMMatrixTranspose(XMLoadFloat4x4(&command.worldMatrix)));
                        float uniformScale = std::max({scale.x, scale.y, scale.z});

                        Tessellation_data tessellationData = this->tessellationData;
                        tessellationData.displacementScale = material->displacementScale * uniformScale;

                        const float normalStrengthMultiplier = 5.0f;
                        tessellationData.normalStrength = material->displacementScale * uniformScale * normalStrengthMultiplier;

                        Texture2D *displacementTexture = material->displacementTexture.Get();
                        tessellationData.texelSize.x = 1.0f / displacementTexture->width;
                        tessellationData.texelSize.y = 1.0f / displacementTexture->height;

                        UploadConstantBuffer(deviceContext, this->tessellationBuffer, tessellationData);

                        ID3D11ShaderResourceView *psSRVs[2] = {
                            material->diffuseTexture.Get()->shaderResourceView,
                            material->normalTexture.Get()->shaderResourceView
                        };
                        deviceContext->PSSetShaderResources(0, 2, psSRVs);

                        ID3D11ShaderResourceView *dsSRVs[1] = {
                            displacementTexture->shaderResourceView
                        };
                        deviceContext->DSSetShaderResources(2, 1, dsSRVs);
                    }

                    const UINT stride = sizeof(Vertex);
                    const UINT offset = 0;
                    deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                    deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                    deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
                }

                deviceContext->HSSetShader(nullptr, nullptr, 0);
                deviceContext->DSSetShader(nullptr, nullptr, 0);
                deviceContext->VSSetShader(this->gBufferVS, nullptr, 0);
                deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                ID3D11ShaderResourceView *nullSRV = nullptr;
                deviceContext->DSSetShaderResources(2, 1, &nullSRV);
            }

            ID3D11RenderTargetView *nullRtvs[3] = {};
            deviceContext->OMSetRenderTargets(3, nullRtvs, nullptr);

            ID3D11ShaderResourceView *nullSRVs[2] = {};
            deviceContext->PSSetShaderResources(0, 2, nullSRVs);
        }
    );
}

void Renderer::RegisterLightingPass(
    FrameGraph::TextureHandle albedoHandle,
    FrameGraph::TextureHandle normalHandle,
    FrameGraph::TextureHandle specularHandle,
    FrameGraph::TextureHandle depthHandle,
    const Shadow_handles &shadowHandles,
    const Reflection_probe_handles &reflectionHandles,
    FrameGraph::TextureHandle lightingOutputHandle,
    FrameGraph::TextureHandle lightingDummyHandle
) {
    struct Lighting_pass_data {
        FrameGraph::TextureHandle albedo;
        FrameGraph::TextureHandle normal;
        FrameGraph::TextureHandle specular;
        FrameGraph::TextureHandle depth;

        FrameGraph::TextureHandle shadowMapDirectional;
        FrameGraph::TextureHandle shadowMapSpot;

        FrameGraph::TextureHandle reflectionProbes;

        FrameGraph::TextureHandle output;

        FrameGraph::TextureHandle dummy;
    };

    this->frameGraph.AddRenderPass<Lighting_pass_data>(
        "Lighting pass",
        [&](Lighting_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.albedo   = builder.Read(albedoHandle);
            data.normal   = builder.Read(normalHandle);
            data.specular = builder.Read(specularHandle);
            data.depth    = builder.Read(depthHandle);

            data.shadowMapDirectional = builder.Read(shadowHandles.shadowMapDirectional);
            data.shadowMapSpot        = builder.Read(shadowHandles.shadowMapSpot);

            data.reflectionProbes = builder.Read(reflectionHandles.reflectionProbes);

            data.output = builder.Write(lightingOutputHandle);

            data.dummy = builder.Write(lightingDummyHandle); // Needed to ensure that this pass executes before the particle pass
        },
        [this, shadowHandles, reflectionHandles](const Lighting_pass_data &data, FrameGraph::ExecutionContext &context) {
            ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

            Render_view *view = context.GetView(View_type::primary);
            if (!view) {
                LogWarn("View was nullptr\n");
                return;
            }

            this->shadowSystem.UploadLightData(
                deviceContext, 
                *view, 
                this->sharedResources, 
                this->reflectionSystem.GetActiveProbeCount()
            );

            this->reflectionSystem.UploadProbeData(deviceContext);

            deviceContext->CSSetShader(this->lightingCS, nullptr, 0);
            deviceContext->CSSetConstantBuffers(0, 1, &this->sharedResources.perFrameBuffer);
            deviceContext->CSSetConstantBuffers(1, 1, &this->sharedResources.lightingBuffer);

            ID3D11ShaderResourceView *skyboxSRV = nullptr;
            if (view->queue.skyboxCommand.has_value())
                if (Texture_cube *cube = view->queue.skyboxCommand->textureCubeHandle.Get())
                    skyboxSRV = cube->shaderResourceView;

            ID3D11ShaderResourceView *srvs[11] = {
                context.GetShaderResourceView(data.albedo),
                context.GetShaderResourceView(data.normal),
                context.GetShaderResourceView(data.specular),
                context.GetShaderResourceView(data.depth),
                context.GetShaderResourceView(data.shadowMapDirectional),
                context.GetShaderResourceView(data.shadowMapSpot),
                shadowHandles.directionalLightBufferSRV,
                shadowHandles.spotLightBufferSRV,
                context.GetShaderResourceView(data.reflectionProbes),
                reflectionHandles.reflectionProbeBufferSRV,
                skyboxSRV
            };
            deviceContext->CSSetShaderResources(0, 11, srvs);

            ID3D11UnorderedAccessView *uav = context.GetUnorderedAccessView(data.output);
            deviceContext->ClearUnorderedAccessViewFloat(uav, this->clearColour);
            deviceContext->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

            UINT groupsX = (this->width  + 7) / 8;
            UINT groupsY = (this->height + 7) / 8;
            deviceContext->Dispatch(groupsX, groupsY, 1);

            ID3D11ShaderResourceView *nullSrvs[11] = {};
            deviceContext->CSSetShaderResources(0, 11, nullSrvs);

            ID3D11UnorderedAccessView *nullUav = nullptr;
            deviceContext->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);

            deviceContext->CSSetShader(nullptr, nullptr, 0);
        }
    );
}

void Renderer::RegisterResolvePass(
    FrameGraph::TextureHandle lightingOutputHandle,
    FrameGraph::TextureHandle albedoHandle,
    FrameGraph::TextureHandle normalHandle,
    FrameGraph::TextureHandle specularHandle,
    FrameGraph::TextureHandle depthHandle
) {
    struct Resolve_pass_data {
        FrameGraph::TextureHandle lightingOutput;

        // Debug
        FrameGraph::TextureHandle albedo;
        FrameGraph::TextureHandle normal;
        FrameGraph::TextureHandle specular;
        FrameGraph::TextureHandle depth;

        FrameGraph::TextureHandle backbuffer;
    };

    this->frameGraph.AddRenderPass<Resolve_pass_data>(
        "Resolve pass",
        [&](Resolve_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.lightingOutput = builder.Read(lightingOutputHandle);

            // Debug
            data.albedo   = builder.Read(albedoHandle);
            data.normal   = builder.Read(normalHandle);
            data.specular = builder.Read(specularHandle);
            data.depth    = builder.Read(depthHandle);

            data.backbuffer = builder.Write(this->backbufferHandle);
            builder.WritesBackbuffer();
        },
        [this](const Resolve_pass_data &data, FrameGraph::ExecutionContext &context) {
            ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

            Render_view *view = context.GetView(View_type::primary);
            if (view) {
                this->currentDebugData.nearPlane = view->nearPlane;
                this->currentDebugData.farPlane  = view->farPlane;
            }

            UploadConstantBuffer(deviceContext, this->debugResolveBuffer, this->currentDebugData);
            deviceContext->PSSetConstantBuffers(3, 1, &this->debugResolveBuffer); // Debug

            ID3D11RenderTargetView *rtv = context.GetRenderTargetView(data.backbuffer);
            deviceContext->ClearRenderTargetView(rtv, this->clearColour);
            deviceContext->OMSetRenderTargets(1, &rtv, nullptr);

            deviceContext->VSSetShader(this->resolveVS, nullptr, 0);
            deviceContext->PSSetShader(this->resolvePS, nullptr, 0);
            deviceContext->IASetInputLayout(nullptr);
            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            ID3D11ShaderResourceView *srv = context.GetShaderResourceView(data.lightingOutput);
            deviceContext->PSSetShaderResources(0, 1, &srv);

            // Debug
            ID3D11ShaderResourceView *debugSrvs[4] = {
                context.GetShaderResourceView(data.albedo),
                context.GetShaderResourceView(data.normal),
                context.GetShaderResourceView(data.specular),
                context.GetShaderResourceView(data.depth)
            };
            deviceContext->PSSetShaderResources(1, 4, debugSrvs);

            deviceContext->Draw(3, 0);

            ID3D11ShaderResourceView *nullDebugSrvs[4] = {};
            deviceContext->PSSetShaderResources(1, 4, nullDebugSrvs);

            ID3D11ShaderResourceView *nullSrv = nullptr;
            deviceContext->PSSetShaderResources(0, 1, &nullSrv);

            ID3D11RenderTargetView *nullRtv = nullptr;
            deviceContext->OMSetRenderTargets(1, &nullRtv, nullptr);
        }
    );
}

void Renderer::BuildFrameGraph() {
    this->frameGraph.Clear();

    this->backbufferHandle = this->frameGraph.ImportTexture(
        "Backbuffer", 
        nullptr, 
        this->renderTargetView
    );

    // G-buffers:
    // T0: R8G8B8A8_UNORM    = albedo.rgb | specular exponent / 1000 = 32 bits
    // T1: R10G10B10A2_UNORM = encoded normal | isReflective flag    = 32 bits
    // T2: R11G11B10_FLOAT   = specular colour                       = 32 bits
    //                                                               = 96 bits
    // Position is reconstructed from the depth map
    // TODO: Per-object ambient?

    FrameGraph::Texture_desc desc{};
    desc.sizeMode = FrameGraph::Texture_desc::Size_mode::relative;
    desc.width = desc.height = 1.0f;
    desc.mipLevels = 1;
    desc.bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    auto albedoHandle = this->frameGraph.CreateTexture("GBuffer_albedo", desc);

    desc.format = DXGI_FORMAT_R10G10B10A2_UNORM;
    auto normalHandle = this->frameGraph.CreateTexture("GBuffer_normal", desc);

    desc.format = DXGI_FORMAT_R11G11B10_FLOAT;
    auto specularHandle = this->frameGraph.CreateTexture("GBuffer_specular", desc);

    desc.bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    auto depthHandle = this->frameGraph.CreateTexture("Depth_stencil", desc);

    desc.bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    auto lightingOutputHandle = this->frameGraph.CreateTexture("Lighting_output", desc);

    // Dummy handle is used to ensure correct ordering of the particle pass
    desc.sizeMode = FrameGraph::Texture_desc::Size_mode::absolute;
    desc.width = desc.height = 1.0f;
    desc.bindFlags = 0;
    desc.format = DXGI_FORMAT_R8_UNORM;
    auto lightingDummyHandle = this->frameGraph.CreateTexture("Lighting_dummy", desc);

    Reflection_probe_handles reflectionHandles = this->reflectionSystem.RegisterRenderPasses(
        this->frameGraph, 
        this->sharedResources, 
        this->clearColour
    );

    Shadow_handles shadowHandles = this->shadowSystem.RegisterRenderPasses(
        this->frameGraph, 
        this->sharedResources
    );

    this->RegisterGeometryPass(albedoHandle, normalHandle, specularHandle, depthHandle);

    this->RegisterLightingPass(
        albedoHandle, normalHandle, specularHandle, depthHandle, 
        shadowHandles, reflectionHandles, lightingOutputHandle, lightingDummyHandle
    );

    Particle_handles particleHandles = this->particleSystem.RegisterRenderPasses(
        this->frameGraph, 
        this->sharedResources, 
        this->viewport, 
        depthHandle, 
        lightingOutputHandle, 
        lightingDummyHandle
    );

    this->RegisterResolvePass(lightingOutputHandle, albedoHandle, normalHandle, specularHandle, depthHandle);

    this->frameGraph.Compile(this->width, this->height);
}

Render_view *Renderer::GetView(View_type type, int index) {
    for (Render_view &view : this->views)
        if (view.type == type && view.index == index)
            return &view;

    return nullptr;
}

bool Renderer::Initialize(HWND hWnd) {
    LogInfo("Creating renderer...\n");
    LogIndent();

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    this->width = clientRect.right - clientRect.left;
    this->height = clientRect.bottom - clientRect.top;

    if (!this->CreateInterface(hWnd))
        return false;

    if (!this->CreateRenderTargetView())
        return false;

    if (!this->sharedResources.Initialize(this->device))
        return false;

    if (!this->LoadGBufferShaders())
        return false;

    if (!this->LoadTessellationShaders())
        return false;

    if (!this->LoadLightingShader())
        return false;

    if (!this->LoadResolveShaders())
        return false;

    if (!this->shadowSystem.Initialize(this->device, shaderDir))
        return false;

    if (!this->reflectionSystem.Initialize(this->device, shaderDir, &this->shadowSystem))
        return false;

    if (!this->particleSystem.Initialize(this->device, shaderDir))
        return false;

    if (!this->CreateConstantBuffers())
        return false;

    this->SetViewport(this->width, this->height);

    this->frameGraph.SetDevice(this->device);
    this->BuildFrameGraph();

    LogUnindent();
    return true;
}

bool Renderer::Resize(int width, int height) {
    if (width <= 0 || height <= 0)
        return false;

    LogInfo("\n");
    LogInfo("Resizing window...\n");
    LogIndent();

    SafeRelease(this->renderTargetView);

    this->deviceContext->ClearState();
    this->deviceContext->Flush();

    HRESULT result = this->swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(result)) {
        LogError("Failed to resize swap chain buffers");
        return false;
    }

    if (!this->CreateRenderTargetView())
        return false;

    this->SetViewport(width, height);
    this->width = width;
    this->height = height;

    this->frameGraph.OnResize(width, height);

    LogUnindent();
    return true;
}

void Renderer::NewFrame() {
    this->views.clear();
}

void Renderer::Render(Scene *scene) {
    if (!scene) {
        LogWarn("Scene was nullptr\n");
        return;
    }

    this->frameGraph.UpdateImportedTexture(
        this->backbufferHandle,
        nullptr,
        this->renderTargetView
    );

    this->sharedResources.BindSamplers(this->deviceContext);

    scene->GatherVisibility(this->views);

    const Render_view *primary = this->GetView(View_type::primary);

    if (primary) {
        this->reflectionSystem.PrepareViews(scene, *primary, this->views);
        this->shadowSystem.PrepareViews(scene, *primary, this->views);
    }

    this->frameGraph.Execute(this->deviceContext, this->views);

    // Needed for DebugDraw and ImGui
    this->deviceContext->OMSetRenderTargets(1, &this->renderTargetView, nullptr);

    if (primary) {
        XMMATRIX viewMatrix = XMLoadFloat4x4(&primary->viewMatrix);
        XMMATRIX projectionMatrix = XMLoadFloat4x4(&primary->projectionMatrix);
        XMFLOAT4X4 viewProjectionMatrix;
        XMStoreFloat4x4(&viewProjectionMatrix, XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix)));
        DebugDraw::Render(this->deviceContext, viewProjectionMatrix);
    }
}

void Renderer::Present() {
    this->swapChain->Present(0, 0);
}

void Renderer::AddView(const Render_view &view) {
    if (view.type == View_type::primary) {
        for (Render_view &other : this->views) {
            if (other.type == View_type::primary) {
                other = view;
                return;
            }
        }
    }

    this->views.push_back(view);
}

void Renderer::AddView(const Render_view &view, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix) {
    Render_view v = view;

    XMStoreFloat4x4(&v.viewMatrix, viewMatrix);
    XMStoreFloat4x4(&v.projectionMatrix, projectionMatrix);

    XMMATRIX invViewMatrix = XMMatrixInverse(nullptr, viewMatrix);
    BoundingFrustum::CreateFromMatrix(v.frustum, projectionMatrix);
    v.frustum.Transform(v.frustum, invViewMatrix);

    this->AddView(v);
}

 // Debug
void Renderer::SetDebugData(int debugMode, float nearPlane, float farPlane) {
    this->currentDebugData.debugMode = debugMode;
    this->currentDebugData.nearPlane = nearPlane;
    this->currentDebugData.farPlane = farPlane;
}

// Debug
int Renderer::GetDebugMode() {
    return this->currentDebugData.debugMode;
}

void Renderer::ToggleFullscreen() {
    this->swapChain->SetFullscreenState(!this->IsFullscreened(), nullptr);
}

bool Renderer::IsFullscreened() {
    BOOL state;
    this->swapChain->GetFullscreenState(&state, nullptr);

    return state;
}

void Renderer::SetClearColour(float r, float g, float b, float a) {
    auto clamp = [](float value) { return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value; };
    this->clearColour[0] = clamp(r);
    this->clearColour[1] = clamp(g);
    this->clearColour[2] = clamp(b);
    this->clearColour[3] = clamp(a);
}

const float *Renderer::GetClearColour() const {
    return this->clearColour;
}