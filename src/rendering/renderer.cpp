#include "renderer.hpp"
#include "core/logging.hpp"
#include "resources/asset_manager.hpp"

#include <fstream>
#include <vector>

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

Renderer::~Renderer() {
    this->frameGraph.Clear();

    SafeRelease(this->gBufferVS);
    SafeRelease(this->gBufferPS);
    SafeRelease(this->gBufferLayout);
    SafeRelease(this->lightingCS);
    SafeRelease(this->resolveVS);
    SafeRelease(this->resolvePS);

    SafeRelease(this->perFrameBuffer);
    SafeRelease(this->perObjectBuffer);
    SafeRelease(this->perMaterialBuffer);
    SafeRelease(this->lightingBuffer);
    SafeRelease(this->debugResolveBuffer); // Debug

    SafeRelease(this->spotLightBufferSRV);
    SafeRelease(this->spotLightBuffer);

    for (int i = 0; i < (int)Sampler_state_type::count; ++i)
        SafeRelease(this->samplerStates[i]);

    SafeRelease(this->renderTargetView);

    if (this->deviceContext) {
        this->deviceContext->ClearState();
        this->deviceContext->Flush();

        this->deviceContext->Release();
    }

    if (this->swapChain) {
        this->swapChain->SetFullscreenState(false, nullptr);
        this->swapChain->Release();
    }

    if (this->device) {
#ifdef _DEBUG
        ID3D11Debug *debugDevice{};
        HRESULT result = this->device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debugDevice);

        if (SUCCEEDED(result)) {
            OutputDebugStringW(L"\n[ReportLiveDeviceObjects start]\n");
            debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            OutputDebugStringW(L"[ReportLiveDeviceObjects end]\n\n");

            debugDevice->Release();
        }
#endif

        this->device->Release();
    }
}

bool Renderer::CreateInterface(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC swapChainDesc{};

    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

bool Renderer::CreateConstantBuffers() {
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    bufferDesc.ByteWidth = sizeof(Per_object_data);
    HRESULT result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->perObjectBuffer);
    if (FAILED(result)) {
        LogError("Failed to create per-object constant buffer");
        return false;
    }

    bufferDesc.ByteWidth = sizeof(Per_frame_data);
    result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->perFrameBuffer);
    if (FAILED(result)) {
        LogError("Failed to create per-frame constant buffer");
        return false;
    }

    bufferDesc.ByteWidth = sizeof(Per_material_data);
    result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->perMaterialBuffer);
    if (FAILED(result)) {
        LogError("Failed to create per-material constant buffer");
        return false;
    }

    bufferDesc.ByteWidth = sizeof(Lighting_data);
    result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->lightingBuffer);
    if (FAILED(result)) {
        LogError("Failed to create lighting constant buffer");
        return false;
    }

    // Debug
    bufferDesc.ByteWidth = sizeof(Debug_resolve_data);
    result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->debugResolveBuffer);
    if (FAILED(result)) {
        LogError("Failed to create debug resolve constant buffer");
        return false;
    }

    //bufferDesc.ByteWidth = sizeof(Lighting_data);
    //result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->lightingBuffer);
    //if (FAILED(result)) {
    //    LogError("Failed to create lighting constant buffer");
    //    return false;
    //}

    LogInfo("Constant buffers created\n");

    return true;
}

bool Renderer::CreateStructuredBuffers() {
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = sizeof(Spot_light_data) * Renderer::MAX_SPOT_LIGHTS;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(Spot_light_data);

    HRESULT result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->spotLightBuffer);
    if (FAILED(result)) {
        LogError("Failed to create spot light structured buffer");
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = Renderer::MAX_SPOT_LIGHTS;

    result = this->device->CreateShaderResourceView(this->spotLightBuffer, &srvDesc, &this->spotLightBufferSRV);
    if (FAILED(result)) {
        LogError("Failed to create spot light structured buffer SRV");
        return false;
    }

    LogInfo("Structured buffers created\n");

    return true;
}

bool Renderer::CreateCommonSamplerStates() {
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    HRESULT result = this->device->CreateSamplerState(
        &samplerDesc, 
        &this->samplerStates[(int)Sampler_state_type::linearWrap]
    );
    if (FAILED(result)) {
        LogError("Failed to create linear wrap sampler state");
        return false;
    }

    LogInfo("Common sampler states created\n");

    return true;
}

// TODO: Look this over. Refactor?
static bool LoadShaderBytecode(const std::string &path, std::vector<uint8_t> &out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LogError("Failed to load file '%s'", path.c_str());
        return false;
    }

    size_t size = (size_t)file.tellg();
    out.resize(size);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(out.data()), size);

    return true;
}

bool Renderer::LoadDeferredShaders() {
    const std::string shaderDir = "assets/shaders/";

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

    if (!LoadShaderBytecode(shaderDir + "cs_lighting.cso", bytecode))
        return false;

    result = this->device->CreateComputeShader(bytecode.data(), bytecode.size(), nullptr, &this->lightingCS);
    if (FAILED(result)) {
        LogError("Failed to create lighting compute shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "vs_resolve.cso", bytecode))
        return false;

    result = this->device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->resolveVS);
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

    LogInfo("Loaded shaders for deferred rendering\n");

    return true;
}

void Renderer::SetViewport(int width, int height) {
    this->viewport.TopLeftX = 0.0f;
    this->viewport.TopLeftY = 0.0f;
    this->viewport.Width    = (FLOAT)width;
    this->viewport.Height   = (FLOAT)height;
    this->viewport.MinDepth = 0.0f;
    this->viewport.MaxDepth = 1.0f;
}

void Renderer::BindCommonSamplerStates() {
    this->deviceContext->PSSetSamplers(0, (int)Sampler_state_type::count, this->samplerStates);
}

void Renderer::UploadLightData() {
    Lighting_data lightingData{};

    if (this->renderQueue.directionalLightCommand.has_value()) {
        const Directional_light_command &dlc = this->renderQueue.directionalLightCommand.value();
        lightingData.directionalLightDirection = dlc.direction;
        lightingData.directionalLightColour    = dlc.colour;
        lightingData.directionalLightIntensity = dlc.intensity;
        lightingData.ambientColour             = dlc.ambientColour;
    }

    lightingData.spotLightCount = min(Renderer::MAX_SPOT_LIGHTS, this->renderQueue.spotLightCommands.size());

    this->UploadConstantBuffer<Lighting_data>(this->lightingBuffer, lightingData);

    if (lightingData.spotLightCount <= 0)
        return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result = this->deviceContext->Map(this->spotLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map spot light buffer\n");
        return;
    }

    Spot_light_data *dest = static_cast<Spot_light_data *>(mapped.pData);
    for (int i = 0; i < lightingData.spotLightCount; ++i) {
        const Spot_light_command &src = this->renderQueue.spotLightCommands[i];
        dest[i].position      = src.position;
        dest[i].intensity     = src.intensity;
        dest[i].direction     = src.direction;
        dest[i].range         = src.range;
        dest[i].colour        = src.colour;
        dest[i].cosInnerAngle = cosf(src.innerConeAngle);
        dest[i].cosOuterAngle = cosf(src.outerConeAngle);
        dest[i].castsShadows  = src.castsShadows ? 1 : 0;

        XMStoreFloat4x4(&dest[i].viewProjectionMatrix, XMMatrixIdentity()); // TODO
    }

    this->deviceContext->Unmap(this->spotLightBuffer, 0);
}

void Renderer::BuildFrameGraph() {
    this->frameGraph.Clear();

    this->backbufferHandle = this->frameGraph.ImportTexture("Backbuffer", nullptr, this->renderTargetView);

    // T0: R8G8B8A8_UNORM    = albedo.rgb | specular exponent / 1000 = 32 bits
    // T1: R10G10B10A2_UNORM = encoded normal | unused               = 32 bits
    // T2: R11G11B10_FLOAT   = specular colour                       = 32 bits
    //                                                               = 96 bits
    // Position is reconstructed from the depth map
    // TODO: Per-object ambient?

    FrameGraph::Texture_desc desc{};
    desc.sizeMode    = FrameGraph::Texture_desc::Size_mode::relative;
    desc.widthScale  = 1.0f;
    desc.heightScale = 1.0f;
    desc.mipLevels   = 1;
    desc.bindFlags   = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    auto albedoHandle = this->frameGraph.CreateTexture("GBuffer_albedo", desc);

    desc.format = DXGI_FORMAT_R10G10B10A2_UNORM;
    auto normalHandle = this->frameGraph.CreateTexture("GBuffer_normal", desc);

    desc.format = DXGI_FORMAT_R11G11B10_FLOAT;
    auto specularHandle = this->frameGraph.CreateTexture("GBuffer_specular", desc);

    desc.bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    auto depthHandle = this->frameGraph.CreateTexture("Depth_stencil", desc);

    desc.bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    auto lightingOutputHandle = this->frameGraph.CreateTexture("Lighting_output", desc);

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

            ID3D11RenderTargetView *rtvs[3] = {
                context.GetRenderTargetView(data.albedo),
                context.GetRenderTargetView(data.normal),
                context.GetRenderTargetView(data.specular)
            };

            ID3D11DepthStencilView *dsv = context.GetDepthStencilView(data.depth);

            const float clearAlbedo[4]   = {0.0f, 0.0f, 0.0f, 0.0f};
            const float clearNormal[4]   = {0.5f, 0.5f, 0.1f, 0.0f};
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

            deviceContext->VSSetConstantBuffers(0, 1, &this->perFrameBuffer);

            for (Geometry_command &command : context.GetRenderQueue().geometryCommands) {
                Per_object_data perObjectData{};
                perObjectData.worldMatrix = command.worldMatrix;
                XMStoreFloat4x4(
                    &perObjectData.worldMatrixInvTranspose, 
                    XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&command.worldMatrix)))
                );

                this->UploadConstantBuffer<Per_object_data>(this->perObjectBuffer, perObjectData);
                deviceContext->VSSetConstantBuffers(1, 1, &this->perObjectBuffer);

                Material *material = command.material.Get();
                if (material) {
                    Per_material_data perMaterialData{};
                    perMaterialData.materialAmbient          = material->ambientColour;
                    perMaterialData.materialDiffuse          = material->diffuseColour;
                    perMaterialData.materialSpecular         = material->specularColour;
                    perMaterialData.materialSpecularExponent = material->specularExponent;

                    this->UploadConstantBuffer<Per_material_data>(this->perMaterialBuffer, perMaterialData);
                    deviceContext->PSSetConstantBuffers(2, 1, &this->perMaterialBuffer);

                    ID3D11ShaderResourceView *srvs[2] = {
                        material->diffuseTexture.Get()->shaderResourceView,
                        material->normalTexture.Get()->shaderResourceView
                    };
                    deviceContext->PSSetShaderResources(0, 2, srvs);
                }

                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
            }

            ID3D11RenderTargetView *nullRtvs[3] = {};
            deviceContext->OMSetRenderTargets(3, nullRtvs, nullptr);

            ID3D11ShaderResourceView *nullSrv[2] = {};
            deviceContext->PSSetShaderResources(0, 2, nullSrv);
        }
    );

    struct Lighting_pass_data {
        FrameGraph::TextureHandle albedo;
        FrameGraph::TextureHandle normal;
        FrameGraph::TextureHandle specular;
        FrameGraph::TextureHandle depth;

        FrameGraph::TextureHandle output;
    };

    this->frameGraph.AddRenderPass<Lighting_pass_data>(
        "Lighting pass",
        [&](Lighting_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.albedo   = builder.Read(albedoHandle);
            data.normal   = builder.Read(normalHandle);
            data.specular = builder.Read(specularHandle);
            data.depth    = builder.Read(depthHandle);

            data.output = builder.Write(lightingOutputHandle);
        },
        [this](const Lighting_pass_data &data, FrameGraph::ExecutionContext &context) {
            ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

            deviceContext->CSSetShader(this->lightingCS, nullptr, 0);
            deviceContext->CSSetConstantBuffers(0, 1, &this->perFrameBuffer);
            deviceContext->CSSetConstantBuffers(1, 1, &this->lightingBuffer);

            ID3D11ShaderResourceView *srvs[4] = {
                context.GetShaderResourceView(data.albedo),
                context.GetShaderResourceView(data.normal),
                context.GetShaderResourceView(data.specular),
                context.GetShaderResourceView(data.depth)
            };
            deviceContext->CSSetShaderResources(0, 4, srvs);

            deviceContext->CSSetShaderResources(4, 1, &this->spotLightBufferSRV);

            ID3D11UnorderedAccessView *uav = context.GetUnorderedAccessView(data.output);
            deviceContext->ClearUnorderedAccessViewFloat(uav, this->clearColour);
            deviceContext->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

            UINT groupsX = (this->width  + 7) / 8;
            UINT groupsY = (this->height + 7) / 8;
            deviceContext->Dispatch(groupsX, groupsY, 1);

            ID3D11ShaderResourceView *nullSrvs[5] = {};
            deviceContext->CSSetShaderResources(0, 5, nullSrvs);

            ID3D11UnorderedAccessView *nullUav = nullptr;
            deviceContext->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);

            deviceContext->CSSetShader(nullptr, nullptr, 0);
        }
    );

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

    this->frameGraph.Compile(this->width, this->height);
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

    if (!this->CreateConstantBuffers())
        return false;

    if (!this->CreateStructuredBuffers())
        return false;

    if (!this->CreateCommonSamplerStates())
        return false;

    if (!this->LoadDeferredShaders())
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

void Renderer::Begin() {
    this->renderQueue.Clear();

    this->frameGraph.UpdateImportedTexture(
        this->backbufferHandle,
        nullptr,
        this->renderTargetView
    );

    deviceContext->RSSetViewports(1, &this->viewport);

    this->BindCommonSamplerStates();
}

void Renderer::End() {
    this->UploadConstantBuffer<Per_frame_data>(this->perFrameBuffer, this->currentFrameData);
    this->UploadConstantBuffer<Debug_resolve_data>(this->debugResolveBuffer, this->currentDebugData); // Debug
    this->UploadLightData();

    this->frameGraph.Execute(this->deviceContext, this->renderQueue);
    this->swapChain->Present(0, 0);
}

void Renderer::SetCameraData(const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix, const XMFLOAT3 &position) {
    XMMATRIX viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
    XMMATRIX invViewProjectionMatrix = XMMatrixInverse(nullptr, viewProjectionMatrix);

    XMStoreFloat4x4(&this->currentFrameData.viewMatrix, XMMatrixTranspose(viewMatrix));
    XMStoreFloat4x4(&this->currentFrameData.projectionMatrix, XMMatrixTranspose(projectionMatrix));
    XMStoreFloat4x4(&this->currentFrameData.viewProjectionMatrix, XMMatrixTranspose(viewProjectionMatrix));
    XMStoreFloat4x4(&this->currentFrameData.invViewProjectionMatrix, XMMatrixTranspose(invViewProjectionMatrix));

    this->currentFrameData.cameraPosition = position;
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

ID3D11Device *Renderer::GetDevice() const {
    return this->device;
}

int Renderer::GetWidth() const {
    return this->width;
}

int Renderer::GetHeight() const {
    return this->height;
}

float Renderer::GetAspectRatio() const {
    return (float)this->width / this->height;
}

void Renderer::ToggleFullscreen() {
    this->swapChain->SetFullscreenState(!this->IsFullscreened(), nullptr);
}

bool Renderer::IsFullscreened() {
    BOOL state;
    this->swapChain->GetFullscreenState(&state, nullptr);

    return state;
}

inline float Clamp(float value, float min, float max) {
    return value < min ? min : value > max ? max : value;
}

void Renderer::SetClearColour(float r, float g, float b, float a) {
    this->clearColour[0] = Clamp(r, 0.0f, 1.0f);
    this->clearColour[1] = Clamp(g, 0.0f, 1.0f);
    this->clearColour[2] = Clamp(b, 0.0f, 1.0f);
    this->clearColour[3] = Clamp(a, 0.0f, 1.0f);
}

const float *Renderer::GetClearColour() const {
    return this->clearColour;
}

RenderQueue &Renderer::GetRenderQueue() {
    return this->renderQueue;
}

FrameGraph &Renderer::GetFrameGraph() {
    return this->frameGraph;
}

FrameGraph::TextureHandle Renderer::GetBackbufferHandle() const {
    return this->backbufferHandle;
}