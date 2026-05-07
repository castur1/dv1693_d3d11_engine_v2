#include "renderer.hpp"
#include "core/logging.hpp"
#include "resources/asset_manager.hpp"
#include "scene/scene.hpp"
#include "debugging/debug_draw.hpp"

#include <fstream>
#include <vector>

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

static const std::string shaderDir = "assets/shaders/";

Renderer::~Renderer() {
    this->frameGraph.Clear();

    SafeRelease(this->skyboxCS);

    SafeRelease(this->reflectionVS);
    SafeRelease(this->reflectionPS);
    SafeRelease(this->reflectionLayout);

    SafeRelease(this->reflectionProbeSRV);
    for (int i = 0; i < MAX_REFLECTION_PROBES * 6; ++i) {
        SafeRelease(this->reflectionProbeRTVs[i]);
        SafeRelease(this->reflectionProbeUAVs[i]);
    }
    SafeRelease(this->reflectionProbeTexture);
    SafeRelease(this->reflectionProbeDepthDSV);
    SafeRelease(this->reflectionProbeDepth);

    SafeRelease(this->reflectionProbeBufferSRV);
    SafeRelease(this->reflectionProbeBuffer);

    SafeRelease(this->shadowVS);
    SafeRelease(this->shadowLayout);
    SafeRelease(this->shadowRS);
    SafeRelease(this->shadowSampler);
    SafeRelease(this->gBufferVS);
    SafeRelease(this->gBufferPS);
    SafeRelease(this->gBufferLayout);
    SafeRelease(this->lightingCS);
    SafeRelease(this->resolveVS);
    SafeRelease(this->resolvePS);

    SafeRelease(this->shadowMapDirectionalSRV);
    for (int i = 0; i < MAX_DIRECTIONAL_SHADOW_MAPS; ++i)
        SafeRelease(this->shadowMapDirectionalDSVs[i]);
    SafeRelease(this->shadowMapDirectionalTexture);

    SafeRelease(this->shadowMapSpotSRV);
    for (int i = 0; i < MAX_SPOT_SHADOW_MAPS; ++i)
        SafeRelease(this->shadowMapSpotDSVs[i]);
    SafeRelease(this->shadowMapSpotTexture);

    SafeRelease(this->shadowBuffer);
    SafeRelease(this->perFrameBuffer);
    SafeRelease(this->perObjectBuffer);
    SafeRelease(this->perMaterialBuffer);
    SafeRelease(this->lightingBuffer);
    SafeRelease(this->debugResolveBuffer); // Debug

    SafeRelease(this->directionalLightBufferSRV);
    SafeRelease(this->directionalLightBuffer);
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

    LogInfo("Constant buffers created\n");

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

static bool CreateDepthArray(
    ID3D11Device *device,
    int resolution,
    int arraySize,
    ID3D11Texture2D **outTexture,
    ID3D11DepthStencilView **outDSVs,
    ID3D11ShaderResourceView **outSRV,
    const char *debugName
) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = resolution;
    desc.Height = resolution;
    desc.MipLevels = 1;
    desc.ArraySize = arraySize;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    HRESULT result = device->CreateTexture2D(&desc, nullptr, outTexture);
    if (FAILED(result)) {
        LogError("Failed to create depth texture array '%s'", debugName);
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    dsvDesc.Texture2DArray.MipSlice = 0;
    dsvDesc.Texture2DArray.ArraySize = 1;

    for (int i = 0; i < arraySize; ++i) {
        dsvDesc.Texture2DArray.FirstArraySlice = i;

        result = device->CreateDepthStencilView(*outTexture, &dsvDesc, &outDSVs[i]);
        if (FAILED(result)) {
            LogError("Failed to create DSV[%d] for '%s'", i, debugName);
            return false;
        }
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = arraySize;

    result = device->CreateShaderResourceView(*outTexture, &srvDesc, outSRV);
    if (FAILED(result)) {
        LogError("Failed to create SRV for '%s'", debugName);
        return false;
    }

    return true;
}

static bool CreateStructuredBuffer(
    ID3D11Device *device, 
    UINT elementSize, 
    UINT elementCount, 
    ID3D11Buffer **outBuffer, 
    ID3D11ShaderResourceView **outSRV, 
    const char *debugName
) {
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = elementSize * elementCount;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = elementSize;

    HRESULT result = device->CreateBuffer(&desc, nullptr, outBuffer);
    if (FAILED(result)) {
        LogError("Failed to create structured buffer '%s'", debugName);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = elementCount;

    result = device->CreateShaderResourceView(*outBuffer, &srvDesc, outSRV);
    if (FAILED(result)) {
        LogError("Failed to create SRV for '%s'", debugName);
        return false;
    }

    return true;
}

bool Renderer::CreateShadowResources() {
    if (!CreateDepthArray(
        this->device,
        SHADOW_MAP_DIRECTIONAL_RESOLUTION,
        MAX_DIRECTIONAL_SHADOW_MAPS,
        &this->shadowMapDirectionalTexture,
        this->shadowMapDirectionalDSVs,
        &this->shadowMapDirectionalSRV,
        "directional"
    )) {
        return false;
    }

    if (!CreateStructuredBuffer(
        this->device,
        sizeof(Directional_light_data),
        MAX_DIRECTIONAL_LIGHTS,
        &this->directionalLightBuffer,
        &this->directionalLightBufferSRV,
        "directional"
    )) {
        return false;
    }

    if (!CreateDepthArray(
        this->device,
        SHADOW_MAP_SPOT_RESOLUTION,
        MAX_SPOT_SHADOW_MAPS,
        &this->shadowMapSpotTexture,
        this->shadowMapSpotDSVs,
        &this->shadowMapSpotSRV,
        "spot"
    )) {
        return false;
    }

    if (!CreateStructuredBuffer(
        this->device,
        sizeof(Spot_light_data),
        MAX_SPOT_LIGHTS,
        &this->spotLightBuffer,
        &this->spotLightBufferSRV,
        "spot"
    )) {
        return false;
    }

    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_shadow.cso", bytecode))
        return false;

    HRESULT result = this->device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->shadowVS);
    if (FAILED(result)) {
        LogError("Failed to create vertex shader");
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    result = this->device->CreateInputLayout(layoutDesc, 1, bytecode.data(), bytecode.size(), &this->shadowLayout);
    if (FAILED(result)) {
        LogError("Failed to create input layout");
        return false;
    }

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    result = this->device->CreateBuffer(&bufferDesc, nullptr, &this->shadowBuffer);
    if (FAILED(result)) {
        LogError("Failed to create constant buffer");
        return false;
    }

    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK; // TODO: D3D11_CULL_FRONT?
    rasterizerDesc.DepthBias = 1000;
    rasterizerDesc.SlopeScaledDepthBias = 1.0f;
    rasterizerDesc.DepthBiasClamp = 0.01f;
    rasterizerDesc.DepthClipEnable = true;

    result = this->device->CreateRasterizerState(&rasterizerDesc, &this->shadowRS);
    if (FAILED(result)) {
        LogError("Failed to create rasterizer state");
        return false;
    }

    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = this->device->CreateSamplerState(&samplerDesc, &this->shadowSampler);
    if (FAILED(result)) {
        LogError("Failed to create sampler");
        return false;
    }

    LogInfo("Shadow resources created\n");

    return true;
}

bool Renderer::CreateReflectionProbeResources() {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = REFLECTION_PROBE_RESOLUTION;
    desc.Height = REFLECTION_PROBE_RESOLUTION;
    desc.MipLevels = 0;
    desc.ArraySize = MAX_REFLECTION_PROBES * 6;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HRESULT result = this->device->CreateTexture2D(&desc, nullptr, &this->reflectionProbeTexture);
    if (FAILED(result)) {
        LogError("Failed to create texture cube array");
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
    srvDesc.TextureCubeArray.MostDetailedMip = 0;
    srvDesc.TextureCubeArray.MipLevels = -1;
    srvDesc.TextureCubeArray.First2DArrayFace = 0;
    srvDesc.TextureCubeArray.NumCubes = MAX_REFLECTION_PROBES;

    result = this->device->CreateShaderResourceView(this->reflectionProbeTexture, &srvDesc, &this->reflectionProbeSRV);
    if (FAILED(result)) {
        LogError("Failed to create SRV");
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = desc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    rtvDesc.Texture2DArray.MipSlice = 0;
    rtvDesc.Texture2DArray.ArraySize = 1;

    for (int i = 0; i < MAX_REFLECTION_PROBES * 6; ++i) {
        rtvDesc.Texture2DArray.FirstArraySlice = i;
        result = this->device->CreateRenderTargetView(this->reflectionProbeTexture, &rtvDesc, &this->reflectionProbeRTVs[i]);
        if (FAILED(result)) {
            LogError("Failed to create RTV[%d]", i);
            return false;
        }
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = desc.Format;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    uavDesc.Texture2DArray.MipSlice = 0;
    uavDesc.Texture2DArray.ArraySize = 1;

    for (int i = 0; i < MAX_REFLECTION_PROBES * 6; ++i) {
        uavDesc.Texture2DArray.FirstArraySlice = i;
        result = this->device->CreateUnorderedAccessView(this->reflectionProbeTexture, &uavDesc, &this->reflectionProbeUAVs[i]);
        if (FAILED(result)) {
            LogError("Failed to create UAV[%d]", i);
            return false;
        }
    }

    D3D11_TEXTURE2D_DESC dsDesc{};
    dsDesc.Width = REFLECTION_PROBE_RESOLUTION;
    dsDesc.Height = REFLECTION_PROBE_RESOLUTION;
    dsDesc.MipLevels = 1;
    dsDesc.ArraySize = 1;
    dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.Usage = D3D11_USAGE_DEFAULT;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    result = this->device->CreateTexture2D(&dsDesc, nullptr, &this->reflectionProbeDepth);
    if (FAILED(result)) {
        LogError("Failed to create depth stencil texture");
        return false;
    }

    result = this->device->CreateDepthStencilView(this->reflectionProbeDepth, nullptr, &this->reflectionProbeDepthDSV);
    if (FAILED(result)) {
        LogError("Failed to create DSV");
        return false;
    }

    if (!CreateStructuredBuffer(
        this->device,
        sizeof(Reflection_probe_data),
        MAX_REFLECTION_PROBES,
        &this->reflectionProbeBuffer,
        &this->reflectionProbeBufferSRV,
        "reflection"
    )) {
        return false;
    }

    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_reflection.cso", bytecode))
        return false;

    result = this->device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->reflectionVS);
    if (FAILED(result)) {
        LogError("Failed to create vertex shader");
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    result = this->device->CreateInputLayout(layoutDesc, 4, bytecode.data(), bytecode.size(), &this->reflectionLayout);
    if (FAILED(result)) {
        LogError("Failed to create input layout");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ps_reflection.cso", bytecode))
        return false;

    result = this->device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &this->reflectionPS);
    if (FAILED(result)) {
        LogError("Failed to create pixel shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "cs_skybox.cso", bytecode))
        return false;

    result = this->device->CreateComputeShader(bytecode.data(), bytecode.size(), nullptr, &this->skyboxCS);
    if (FAILED(result)) {
        LogError("Failed to create skybox compute shader");
        return false;
    }

    LogInfo("Reflection probe resources created\n");

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
    this->deviceContext->CSSetSamplers(0, (int)Sampler_state_type::count, this->samplerStates);
}

void Renderer::UploadPerFrameData(const Render_view &view) {
    Per_frame_data data{};
    data.viewMatrix              = view.viewMatrix;
    data.projectionMatrix        = view.projectionMatrix;
    data.viewProjectionMatrix    = view.viewProjectionMatrix;
    data.invViewProjectionMatrix = view.invViewProjectionMatrix;
    data.cameraPosition          = view.cameraPosition;

    this->UploadConstantBuffer(this->perFrameBuffer, data);
}

void Renderer::UploadLightData(const Render_view &view) {
    Lighting_data lightingData{};
    lightingData.directionalLightCount = view.queue.directionalLightCommands.size();
    lightingData.spotLightCount = view.queue.spotLightCommands.size();
    lightingData.reflectionProbeCount = this->perFrameReflectionProbeData.count;
    lightingData.hasSkybox = view.queue.skyboxCommand.has_value() ? 1 : 0;

    for (const auto &dlc : view.queue.directionalLightCommands) {
        lightingData.ambientColour.x += dlc.ambientColour.x;
        lightingData.ambientColour.y += dlc.ambientColour.y;
        lightingData.ambientColour.z += dlc.ambientColour.z;
    }

    this->UploadConstantBuffer(this->lightingBuffer, lightingData);

    // Directional

    int directionalCommandToSlot[MAX_DIRECTIONAL_LIGHTS];
    memset(directionalCommandToSlot, -1, sizeof(directionalCommandToSlot));
    for (int i = 0; i < this->perFrameShadowData.directionalCount; ++i)
        directionalCommandToSlot[this->perFrameShadowData.directionalSlotToCommand[i]] = i;

    int directionalCount = min(MAX_DIRECTIONAL_LIGHTS, view.queue.directionalLightCommands.size());

    Directional_light_data directionalData[MAX_DIRECTIONAL_LIGHTS];
    for (int i = 0; i < directionalCount; ++i) {
        const Directional_light_command &dlc = view.queue.directionalLightCommands[i];
        Directional_light_data &entry = directionalData[i];

        entry.direction        = dlc.direction;
        entry.intensity        = dlc.intensity;
        entry.colour           = dlc.colour;
        entry.castsShadows     = dlc.castsShadows ? 1 : 0;
        entry.shadowSliceIndex = directionalCommandToSlot[i];

        if (entry.shadowSliceIndex >= 0)
            entry.viewProjectionMatrix = this->perFrameShadowData.directionalViewProjectionMatrices[entry.shadowSliceIndex];
        else
            XMStoreFloat4x4(&entry.viewProjectionMatrix, XMMatrixIdentity());
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result = this->deviceContext->Map(this->directionalLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map directional light buffer\n");
        return;
    }

    memcpy(mapped.pData, directionalData, sizeof(Directional_light_data) * directionalCount);
    this->deviceContext->Unmap(this->directionalLightBuffer, 0);
    
    // Spot

    int spotCommandToSlot[MAX_SPOT_LIGHTS];
    memset(spotCommandToSlot, -1, sizeof(spotCommandToSlot));
    for (int i = 0; i < this->perFrameShadowData.spotCount; ++i)
        spotCommandToSlot[this->perFrameShadowData.spotSlotToCommand[i]] = i;

    int spotCount = min(MAX_SPOT_LIGHTS, view.queue.spotLightCommands.size());

    Spot_light_data spotData[MAX_SPOT_LIGHTS];
    for (int i = 0; i < spotCount; ++i) {
        const Spot_light_command &slc = view.queue.spotLightCommands[i];
        Spot_light_data &entry = spotData[i];

        entry.position         = slc.position;
        entry.intensity        = slc.intensity;
        entry.direction        = slc.direction;
        entry.range            = slc.range;
        entry.colour           = slc.colour;
        entry.cosInnerAngle    = cosf(slc.innerConeAngle);
        entry.cosOuterAngle    = cosf(slc.outerConeAngle);
        entry.castsShadows     = slc.castsShadows ? 1 : 0;
        entry.shadowSliceIndex = spotCommandToSlot[i];

        if (entry.shadowSliceIndex >= 0)
            entry.viewProjectionMatrix = this->perFrameShadowData.spotViewProjectionMatrices[entry.shadowSliceIndex];
        else
            XMStoreFloat4x4(&entry.viewProjectionMatrix, XMMatrixIdentity());
    }

    result = this->deviceContext->Map(this->spotLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map spot light buffer\n");
        return;
    }

    memcpy(mapped.pData, spotData, sizeof(Spot_light_data) * spotCount);
    this->deviceContext->Unmap(this->spotLightBuffer, 0);
}

void Renderer::UploadReflectionProbeData() {
    if (this->perFrameReflectionProbeData.count <= 0)
        return;

    Reflection_probe_data data[MAX_REFLECTION_PROBES];
    for (int i = 0; i < this->perFrameReflectionProbeData.count; ++i) {
        data[i].position = this->perFrameReflectionProbeData.entries[i].position;
        data[i].radius = this->perFrameReflectionProbeData.entries[i].radius;
        data[i].slotIndex = i;
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result = this->deviceContext->Map(this->reflectionProbeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map reflection probe buffer\n");
        return;
    }

    memcpy(mapped.pData, data, sizeof(Reflection_probe_data) * this->perFrameReflectionProbeData.count);
    this->deviceContext->Unmap(this->reflectionProbeBuffer, 0);
}

void Renderer::BuildFrameGraph() {
    this->frameGraph.Clear();

    this->backbufferHandle = this->frameGraph.ImportTexture(
        "Backbuffer", 
        nullptr, 
        this->renderTargetView
    );

    auto shadowMapDirectionalHandle = this->frameGraph.ImportTexture(
        "ShadowMap_directional",
        this->shadowMapDirectionalTexture,
        nullptr,
        this->shadowMapDirectionalSRV,
        this->shadowMapDirectionalDSVs[0]
    );

    auto shadowMapSpotHandle = this->frameGraph.ImportTexture(
        "ShadowMap_spot",
        this->shadowMapSpotTexture,
        nullptr,
        this->shadowMapSpotSRV,
        this->shadowMapSpotDSVs[0]
    );

    auto reflectionProbeHandle = this->frameGraph.ImportTexture(
        "ReflectionProbe", 
        this->reflectionProbeTexture, 
        nullptr, 
        this->reflectionProbeSRV
    );

    // G-buffers:
    // T0: R8G8B8A8_UNORM    = albedo.rgb | specular exponent / 1000 = 32 bits
    // T1: R10G10B10A2_UNORM = encoded normal | unused               = 32 bits
    // T2: R11G11B10_FLOAT   = specular colour                       = 32 bits
    //                                                               = 96 bits
    // Position is reconstructed from the depth map
    // TODO: Per-object ambient?

    FrameGraph::Texture_desc desc{};
    desc.sizeMode = FrameGraph::Texture_desc::Size_mode::relative;
    desc.width = 1.0f;
    desc.height = 1.0f;
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

    desc.bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    auto lightingOutputHandle = this->frameGraph.CreateTexture("Lighting_output", desc);

    struct Reflection_pass_data {
        FrameGraph::TextureHandle reflectionProbes;
    };

    this->frameGraph.AddRenderPass<Reflection_pass_data>(
        "Reflection pass",
        [&](Reflection_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.reflectionProbes = builder.Write(reflectionProbeHandle);
        },
        [this](const Reflection_pass_data &data, FrameGraph::ExecutionContext &context) {
            ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

            if (this->perFrameReflectionProbeData.count <= 0)
                return;

            Render_view *primaryView = context.GetView(View_type::primary);
            if (!primaryView) {
                LogWarn("Primary view was nullptr\n");
                return;
            }

            this->UploadLightData(*primaryView);

            ID3D11ShaderResourceView *skyboxSRV = nullptr;
            if (primaryView->queue.skyboxCommand.has_value())
                skyboxSRV = primaryView->queue.skyboxCommand->textureCubeHandle.Get()->shaderResourceView;

            if (skyboxSRV) {
                deviceContext->CSSetShader(this->skyboxCS, nullptr, 0);
                deviceContext->CSSetConstantBuffers(0, 1, &this->perFrameBuffer);
                deviceContext->CSSetShaderResources(0, 1, &skyboxSRV);
                
                const UINT groups = (REFLECTION_PROBE_RESOLUTION + 7) / 8;

                for (int i = 0; i < this->perFrameReflectionProbeData.count; ++i) {
                    for (int face = 0; face < 6; ++face) {
                        Render_view *view = context.GetView(View_type::cubeFace, i * 6 + face);
                        if (!view)
                            continue;

                        this->UploadPerFrameData(*view);

                        ID3D11UnorderedAccessView *uav = this->reflectionProbeUAVs[i * 6 + face];
                        deviceContext->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
                        deviceContext->Dispatch(groups, groups, 1);
                    }
                }

                deviceContext->CSSetShader(nullptr, nullptr, 0);
                ID3D11ShaderResourceView *nullSRV = nullptr;
                deviceContext->CSGetShaderResources(0, 1, &nullSRV);
                ID3D11UnorderedAccessView *nullUAV = nullptr;
                deviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
            }

            D3D11_VIEWPORT viewport{};
            viewport.Width = viewport.Height = REFLECTION_PROBE_RESOLUTION;
            viewport.MaxDepth = 1.0f;
            deviceContext->RSSetViewports(1, &viewport);

            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            deviceContext->IASetInputLayout(this->reflectionLayout);
            deviceContext->VSSetShader(this->reflectionVS, nullptr, 0);
            deviceContext->PSSetShader(this->reflectionPS, nullptr, 0);
            deviceContext->VSSetConstantBuffers(0, 1, &this->perFrameBuffer);
            deviceContext->VSSetConstantBuffers(1, 1, &this->perObjectBuffer);
            deviceContext->PSSetConstantBuffers(1, 1, &this->lightingBuffer);
            deviceContext->PSSetShaderResources(0, 1, &this->directionalLightBufferSRV);
            
            for (int i = 0; i < this->perFrameReflectionProbeData.count; ++i) {
                for (int face = 0; face < 6; ++face) {
                    Render_view *view = context.GetView(View_type::cubeFace, i * 6 + face);
                    if (!view)
                        continue;

                    ID3D11RenderTargetView *rtv = this->reflectionProbeRTVs[i * 6 + face];

                    if (!skyboxSRV)
                        deviceContext->ClearRenderTargetView(rtv, this->clearColour);

                    deviceContext->ClearDepthStencilView(this->reflectionProbeDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
                    deviceContext->OMSetRenderTargets(1, &rtv, this->reflectionProbeDepthDSV);

                    this->UploadPerFrameData(*view);

                    for (const Geometry_command &command : view->queue.geometryCommands) {
                        Per_object_data perObjectData{};
                        perObjectData.worldMatrix = command.worldMatrix;
                        XMStoreFloat4x4(
                            &perObjectData.worldMatrixInvTranspose, 
                            XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&command.worldMatrix)))
                        );

                        this->UploadConstantBuffer(this->perObjectBuffer, perObjectData);

                        Material *material = command.material.Get();
                        if (material) {
                            // TODO: This doesn't do full Blinn-Phong, only directional light

                            //Per_material_data perMaterialData{};
                            //perMaterialData.materialAmbient          = material->ambientColour;
                            //perMaterialData.materialDiffuse          = material->diffuseColour;
                            //perMaterialData.materialSpecular         = material->specularColour;
                            //perMaterialData.materialSpecularExponent = material->specularExponent;

                            //this->UploadConstantBuffer(this->perMaterialBuffer, perMaterialData);

                            deviceContext->PSSetShaderResources(0, 1, &material->diffuseTexture.Get()->shaderResourceView);
                        }

                        const UINT stride = sizeof(Vertex);
                        const UINT offset = 0;
                        deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                        deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                        deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
                    }
                }
            }

            deviceContext->GenerateMips(this->reflectionProbeSRV);

            deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
            ID3D11ShaderResourceView *nullSRVs[2] = {};
            deviceContext->PSSetShaderResources(0, 2, nullSRVs);
        }
    );

    struct Shadow_pass_data {
        FrameGraph::TextureHandle shadowMapDirectional;
        FrameGraph::TextureHandle shadowMapSpot;
    };

    this->frameGraph.AddRenderPass<Shadow_pass_data>(
        "Shadow pass",
        [&](Shadow_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.shadowMapDirectional = builder.Write(shadowMapDirectionalHandle);
            data.shadowMapSpot        = builder.Write(shadowMapSpotHandle);
        },
        [this](const Shadow_pass_data &data, FrameGraph::ExecutionContext &context) {
            ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

            deviceContext->RSSetState(this->shadowRS);

            deviceContext->IASetInputLayout(this->shadowLayout);
            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            deviceContext->VSSetShader(this->shadowVS, nullptr, 0);
            deviceContext->PSSetShader(nullptr, nullptr, 0);

            D3D11_VIEWPORT viewport{};
            viewport.Width = viewport.Height = SHADOW_MAP_DIRECTIONAL_RESOLUTION;
            viewport.MaxDepth = 1.0f;
            deviceContext->RSSetViewports(1, &viewport);

            deviceContext->VSSetConstantBuffers(0, 1, &this->shadowBuffer);
            deviceContext->VSSetConstantBuffers(1, 1, &this->perObjectBuffer);

            for (int i = 0; i < this->perFrameShadowData.directionalCount; ++i) {
                Render_view *view = context.GetView(View_type::shadowMapDirectional, i);
                if (!view)
                    break;

                deviceContext->ClearDepthStencilView(this->shadowMapDirectionalDSVs[i], D3D11_CLEAR_DEPTH, 1.0f, 0);
                
                deviceContext->OMSetRenderTargets(0, nullptr, this->shadowMapDirectionalDSVs[i]);

                this->UploadConstantBuffer(this->shadowBuffer, view->viewProjectionMatrix);

                for (const Geometry_command &command : view->queue.geometryCommands) {
                    Per_object_data perObjectData{};
                    perObjectData.worldMatrix = command.worldMatrix;
                    this->UploadConstantBuffer(this->perObjectBuffer, perObjectData);

                    const UINT stride = sizeof(Vertex);
                    const UINT offset = 0;
                    deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                    deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                    deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
                }
            }

            viewport.Width = viewport.Height = SHADOW_MAP_SPOT_RESOLUTION;
            deviceContext->RSSetViewports(1, &viewport);

            for (int i = 0; i < this->perFrameShadowData.spotCount; ++i) {
                Render_view *view = context.GetView(View_type::shadowMapSpot, i);
                if (!view)
                    break;

                deviceContext->ClearDepthStencilView(this->shadowMapSpotDSVs[i], D3D11_CLEAR_DEPTH, 1.0f, 0);
                
                deviceContext->OMSetRenderTargets(0, nullptr, this->shadowMapSpotDSVs[i]);

                this->UploadConstantBuffer(this->shadowBuffer, view->viewProjectionMatrix);

                for (const Geometry_command &command : view->queue.geometryCommands) {
                    Per_object_data perObjectData{};
                    perObjectData.worldMatrix = command.worldMatrix;
                    this->UploadConstantBuffer(this->perObjectBuffer, perObjectData);

                    const UINT stride = sizeof(Vertex);
                    const UINT offset = 0;
                    deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                    deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                    deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
                }
            }

            deviceContext->RSSetState(nullptr);
            deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        }
    );

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

            this->UploadPerFrameData(*view);

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
            deviceContext->VSSetConstantBuffers(1, 1, &this->perObjectBuffer);
            deviceContext->PSSetConstantBuffers(2, 1, &this->perMaterialBuffer);

            deviceContext->RSSetViewports(1, &this->viewport);

            for (Geometry_command &command : view->queue.geometryCommands) {
                Per_object_data perObjectData{};
                perObjectData.worldMatrix = command.worldMatrix;
                XMStoreFloat4x4(
                    &perObjectData.worldMatrixInvTranspose, 
                    XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&command.worldMatrix)))
                );

                this->UploadConstantBuffer(this->perObjectBuffer, perObjectData);

                Material *material = command.material.Get();
                if (material) {
                    Per_material_data perMaterialData{};
                    perMaterialData.materialAmbient          = material->ambientColour;
                    perMaterialData.materialDiffuse          = material->diffuseColour;
                    perMaterialData.materialSpecular         = material->specularColour;
                    perMaterialData.materialSpecularExponent = material->specularExponent;

                    this->UploadConstantBuffer(this->perMaterialBuffer, perMaterialData);

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

        FrameGraph::TextureHandle shadowMapDirectional;
        FrameGraph::TextureHandle shadowMapSpot;

        FrameGraph::TextureHandle reflectionProbes;

        FrameGraph::TextureHandle output;
    };

    this->frameGraph.AddRenderPass<Lighting_pass_data>(
        "Lighting pass",
        [&](Lighting_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.albedo   = builder.Read(albedoHandle);
            data.normal   = builder.Read(normalHandle);
            data.specular = builder.Read(specularHandle);
            data.depth    = builder.Read(depthHandle);

            data.shadowMapDirectional = builder.Read(shadowMapDirectionalHandle);
            data.shadowMapSpot        = builder.Read(shadowMapSpotHandle);

            data.reflectionProbes = builder.Read(reflectionProbeHandle);

            data.output = builder.Write(lightingOutputHandle);
        },
        [this](const Lighting_pass_data &data, FrameGraph::ExecutionContext &context) {
            ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

            Render_view *view = context.GetView(View_type::primary);
            if (!view) {
                LogWarn("View was nullptr\n");
                return;
            }

            this->UploadLightData(*view);
            this->UploadReflectionProbeData();

            deviceContext->CSSetShader(this->lightingCS, nullptr, 0);
            deviceContext->CSSetConstantBuffers(0, 1, &this->perFrameBuffer);
            deviceContext->CSSetConstantBuffers(1, 1, &this->lightingBuffer);

            ID3D11ShaderResourceView *skyboxSRV = nullptr;
            if (view->queue.skyboxCommand.has_value())
                skyboxSRV = view->queue.skyboxCommand->textureCubeHandle.Get()->shaderResourceView;

            ID3D11ShaderResourceView *srvs[11] = {
                context.GetShaderResourceView(data.albedo),
                context.GetShaderResourceView(data.normal),
                context.GetShaderResourceView(data.specular),
                context.GetShaderResourceView(data.depth),
                context.GetShaderResourceView(data.shadowMapDirectional),
                context.GetShaderResourceView(data.shadowMapSpot),
                this->directionalLightBufferSRV,
                this->spotLightBufferSRV,
                context.GetShaderResourceView(data.reflectionProbes),
                this->reflectionProbeBufferSRV,
                skyboxSRV
            };
            deviceContext->CSSetShaderResources(0, 11, srvs);

            // TODO: Make common sampler? This currently overwrites the 2nd common sampler, I think
            deviceContext->CSSetSamplers(1, 1, &this->shadowSampler);

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

            this->UploadConstantBuffer(this->debugResolveBuffer, this->currentDebugData);
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

void Renderer::ComputeDirectionalLightMatrices(
    XMMATRIX &outView, 
    XMMATRIX &outProjection, 
    const Directional_light_command &command, 
    const Render_view &primaryView
) {
    XMFLOAT3 corners[8];
    primaryView.frustum.GetCorners(corners);

    float t = min(primaryView.shadowDistance / primaryView.farPlane, 1.0f);
    for (int i = 0; i < 4; ++i)
        XMStoreFloat3(&corners[i + 4], XMVectorLerp(XMLoadFloat3(&corners[i]), XMLoadFloat3(&corners[i + 4]), t));

    XMVECTOR centre = XMVectorZero();
    for (const XMFLOAT3 &corner : corners)
        centre += XMLoadFloat3(&corner);
    centre /= 8.0f;

    float radius = 0.0f;
    for (const XMFLOAT3 &corner : corners) {
        float distance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&corner) - centre));
        radius = max(radius, distance);
    }

    float worldUnitsPerTexel = (2.0f * radius) / SHADOW_MAP_DIRECTIONAL_RESOLUTION;

    XMVECTOR direction = XMVector3Normalize(XMLoadFloat3(&command.direction));
    XMVECTOR up = fabsf(XMVectorGetY(direction)) < 0.99f ? XMVectorSet(0, 1, 0, 0) : XMVectorSet(1, 0, 0, 0);

    XMMATRIX refView = XMMatrixLookToLH(XMVectorZero(), direction, up);
    XMMATRIX invRefView = XMMatrixInverse(nullptr, refView);

    XMFLOAT3 centreRef;
    XMStoreFloat3(&centreRef, XMVector3Transform(centre, refView));

    centreRef.x = floorf(centreRef.x / worldUnitsPerTexel) * worldUnitsPerTexel;
    centreRef.y = floorf(centreRef.y / worldUnitsPerTexel) * worldUnitsPerTexel;
    
    XMVECTOR snappedCentre = XMVector3Transform(XMLoadFloat3(&centreRef), invRefView);
    outView = XMMatrixLookToLH(snappedCentre, direction, up);

    float minZ = FLT_MAX;
    float maxZ = -FLT_MAX;
    for (const XMFLOAT3 &corner : corners) {
        float z = XMVectorGetZ(XMVector3Transform(XMLoadFloat3(&corner), outView));
        minZ = min(minZ, z);
        maxZ = max(maxZ, z);
    }
    minZ -= 500.0f;

    outProjection = XMMatrixOrthographicOffCenterLH(-radius, radius, -radius, radius, minZ, maxZ);
}

void Renderer::ComputeSpotLightMatrices(
    XMMATRIX &outView, 
    XMMATRIX &outProjection, 
    const Spot_light_command &command
) {
    XMVECTOR position = XMLoadFloat3(&command.position);
    XMVECTOR direction = XMVector3Normalize(XMLoadFloat3(&command.direction));
    XMVECTOR up = fabsf(XMVectorGetY(direction)) < 0.99f ? XMVectorSet(0, 1, 0, 0) : XMVectorSet(1, 0, 0, 0);

    outView = XMMatrixLookToLH(position, direction, up);
    outProjection = XMMatrixPerspectiveFovLH(command.outerConeAngle * 2.0f, 1.0f, 0.1f, command.range);
}

Render_view *Renderer::GetView(View_type type, int index) {
    for (Render_view &view : this->views)
        if (view.type == type && view.index == index)
            return &view;

    return nullptr;
}

void Renderer::SetupShadowViews(Scene *scene) {
    if (!scene)
        return;

    Render_view *primaryView = this->GetView(View_type::primary);
    if (!primaryView)
        return;

    this->perFrameShadowData.directionalCount = 0;
    this->perFrameShadowData.spotCount = 0;

    std::vector<Render_view> shadowViews;

    for (int i = 0; i < primaryView->queue.directionalLightCommands.size(); ++i) {
        if (this->perFrameShadowData.directionalCount >= MAX_DIRECTIONAL_SHADOW_MAPS)
            break;

        const Directional_light_command &command = primaryView->queue.directionalLightCommands[i];
        if (!command.castsShadows)
            continue;

        int slot = this->perFrameShadowData.directionalCount;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        this->ComputeDirectionalLightMatrices(viewMatrix, projectionMatrix, command, *primaryView);
        XMMATRIX viewProjectionMatrix = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix));

        Render_view view{};
        view.type = View_type::shadowMapDirectional;
        view.index = slot;
        view.skipFrustumCulling = true;
        XMStoreFloat4x4(&view.viewProjectionMatrix, viewProjectionMatrix);

        shadowViews.push_back(view);

        XMStoreFloat4x4(
            &this->perFrameShadowData.directionalViewProjectionMatrices[slot], 
            viewProjectionMatrix
        );
        this->perFrameShadowData.directionalSlotToCommand[slot] = i;
        ++this->perFrameShadowData.directionalCount;
    }

    for (int i = 0; i < primaryView->queue.spotLightCommands.size(); ++i) {
        if (this->perFrameShadowData.spotCount >= MAX_SPOT_SHADOW_MAPS)
            break;

        const Spot_light_command &command = primaryView->queue.spotLightCommands[i];
        if (!command.castsShadows)
            continue;

        int slot = this->perFrameShadowData.spotCount;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        this->ComputeSpotLightMatrices(viewMatrix, projectionMatrix, command);
        XMMATRIX viewProjectionMatrix = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix));

        Render_view view{};
        view.type = View_type::shadowMapSpot;
        view.index = slot;
        XMStoreFloat4x4(&view.viewProjectionMatrix, viewProjectionMatrix);

        BoundingFrustum::CreateFromMatrix(view.frustum, projectionMatrix);
        view.frustum.Transform(view.frustum, XMMatrixInverse(nullptr, viewMatrix));

        shadowViews.push_back(view);

        XMStoreFloat4x4(
            &this->perFrameShadowData.spotViewProjectionMatrices[slot], 
            viewProjectionMatrix
        );
        this->perFrameShadowData.spotSlotToCommand[slot] = i;
        ++this->perFrameShadowData.spotCount;
    }

    scene->GatherVisibility(shadowViews);
    this->views.insert(this->views.end(), shadowViews.begin(), shadowViews.end());
}

void Renderer::SetupReflectionProbeViews(Scene *scene) {
    if (!scene)
        return;

    Render_view *primaryView = this->GetView(View_type::primary);
    if (!primaryView)
        return;

    this->perFrameReflectionProbeData.count = 0;

    const auto &reflectionProbeCommands = primaryView->queue.reflectionProbeCommands;
    if (reflectionProbeCommands.empty())
        return;

    struct Face {
        XMFLOAT3 forward;
        XMFLOAT3 up;
    };
    static constexpr Face faces[6] = {
        {{ 1,  0,  0}, {0,  1,  0}}, // +x
        {{-1,  0,  0}, {0,  1,  0}}, // -x
        {{ 0,  1,  0}, {0,  0, -1}}, // +y
        {{ 0, -1,  0}, {0,  0,  1}}, // -y
        {{ 0,  0,  1}, {0,  1,  0}}, // +z
        {{ 0,  0, -1}, {0,  1,  0}}  // -z
    };

    std::vector<Render_view> reflectionProbeViews;

    for (int i = 0; i < reflectionProbeCommands.size(); ++i) {
        if (this->perFrameReflectionProbeData.count >= MAX_REFLECTION_PROBES)
            break;

        const Reflection_probe_command &command = reflectionProbeCommands[i];
        int slot = this->perFrameReflectionProbeData.count;

        XMVECTOR position = XMLoadFloat3(&command.position);
        XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, command.nearPlane, command.farPlane);

        for (int face = 0; face < 6; ++face) {
            XMVECTOR forward = XMLoadFloat3(&faces[face].forward);
            XMVECTOR up = XMLoadFloat3(&faces[face].up);

            XMMATRIX viewMatrix = XMMatrixLookToLH(position, forward, up);
            XMMATRIX viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
            XMMATRIX invViewProjectionMatrix = XMMatrixInverse(nullptr, viewProjectionMatrix);

            Render_view view{};
            view.type = View_type::cubeFace;
            view.index = slot * 6 + face;

            XMStoreFloat4x4(&view.viewMatrix, XMMatrixTranspose(viewMatrix));
            XMStoreFloat4x4(&view.projectionMatrix, XMMatrixTranspose(projectionMatrix));
            XMStoreFloat4x4(&view.viewProjectionMatrix, XMMatrixTranspose(viewProjectionMatrix));
            XMStoreFloat4x4(&view.invViewProjectionMatrix, XMMatrixTranspose(invViewProjectionMatrix));

            view.cameraPosition = command.position;
            view.nearPlane = command.nearPlane;
            view.farPlane = command.farPlane;

            BoundingFrustum::CreateFromMatrix(view.frustum, projectionMatrix);
            view.frustum.Transform(view.frustum, XMMatrixInverse(nullptr, viewMatrix));

            reflectionProbeViews.push_back(view);
        }

        this->perFrameReflectionProbeData.entries[slot] = {command.position, command.radius};

        ++this->perFrameReflectionProbeData.count;
    }

    scene->GatherVisibility(reflectionProbeViews);
    this->views.insert(this->views.end(), reflectionProbeViews.begin(), reflectionProbeViews.end());
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

    if (!this->CreateCommonSamplerStates())
        return false;

    if (!this->LoadDeferredShaders())
        return false;

    if (!this->CreateShadowResources())
        return false;

    if (!this->CreateReflectionProbeResources())
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

    this->BindCommonSamplerStates();

    scene->GatherVisibility(this->views);
    this->SetupReflectionProbeViews(scene);
    this->SetupShadowViews(scene);
    this->frameGraph.Execute(this->deviceContext, this->views);

    // Needed for DebugDraw and ImGui
    this->deviceContext->OMSetRenderTargets(1, &this->renderTargetView, nullptr);

    const Render_view *primary = this->GetView(View_type::primary);
    if (primary)
        DebugDraw::Render(this->deviceContext, primary->viewProjectionMatrix);
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

void Renderer::AddView(const Render_view &view, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix, const XMFLOAT3 &position) {
    Render_view v = view;

    XMMATRIX worldMatrix = XMMatrixInverse(nullptr, viewMatrix);

    XMMATRIX viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
    XMMATRIX invViewProjectionMatrix = XMMatrixInverse(nullptr, viewProjectionMatrix);

    XMStoreFloat4x4(&v.viewMatrix, XMMatrixTranspose(viewMatrix));
    XMStoreFloat4x4(&v.projectionMatrix, XMMatrixTranspose(projectionMatrix));
    XMStoreFloat4x4(&v.viewProjectionMatrix, XMMatrixTranspose(viewProjectionMatrix));
    XMStoreFloat4x4(&v.invViewProjectionMatrix, XMMatrixTranspose(invViewProjectionMatrix));

    v.cameraPosition = position;

    BoundingFrustum::CreateFromMatrix(v.frustum, projectionMatrix);
    v.frustum.Transform(v.frustum, worldMatrix);

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

ID3D11Device *Renderer::GetDevice() const {
    return this->device;
}

ID3D11DeviceContext *Renderer::GetDeviceContext() const {
    return this->deviceContext;
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

std::vector<Render_view> &Renderer::GetViews() {
    return this->views;
}

FrameGraph &Renderer::GetFrameGraph() {
    return this->frameGraph;
}

FrameGraph::TextureHandle Renderer::GetBackbufferHandle() const {
    return this->backbufferHandle;
}