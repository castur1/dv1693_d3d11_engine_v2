#include "reflection_probe_system.hpp"
#include "core/logging.hpp"
#include "rendering/render_utils.hpp"
#include "scene/scene.hpp"
#include "rendering/shadow_system.hpp"

void ReflectionProbeSystem::ExectuteReflectionRenderPass(
    FrameGraph::ExecutionContext &context,
    const SharedResources &sharedResources,
    const float *clearColour
) {
    ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

    if (this->perFrameProbeData.count <= 0)
        return;

    Render_view *primaryView = context.GetView(View_type::primary);
    if (!primaryView) {
        LogWarn("Primary view was nullptr\n");
        return;
    }

    this->shadowSystem->UploadLightData(deviceContext, *primaryView, sharedResources, this->GetActiveProbeCount());

    ID3D11ShaderResourceView *skyboxSRV = nullptr;
    if (primaryView->queue.skyboxCommand.has_value())
        if (Texture_cube *cube = primaryView->queue.skyboxCommand->textureCubeHandle.Get())
            skyboxSRV = cube->shaderResourceView;

    if (skyboxSRV) {
        deviceContext->CSSetShader(this->skyboxCS, nullptr, 0);
        deviceContext->CSSetConstantBuffers(0, 1, &sharedResources.perFrameBuffer);
        deviceContext->CSSetShaderResources(0, 1, &skyboxSRV);

        const UINT groups = (REFLECTION_PROBE_RESOLUTION + 7) / 8;

        for (int i = 0; i < this->perFrameProbeData.count; ++i) {
            for (int face = 0; face < 6; ++face) {
                Render_view *view = context.GetView(View_type::cubeFace, i * 6 + face);
                if (!view)
                    continue;

                sharedResources.UploadPerFrameData(deviceContext, *view);

                ID3D11UnorderedAccessView *uav = this->probeUAVs[i * 6 + face];
                deviceContext->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
                deviceContext->Dispatch(groups, groups, 1);
            }
        }

        deviceContext->CSSetShader(nullptr, nullptr, 0);
        ID3D11ShaderResourceView *nullSRV = nullptr;
        deviceContext->CSSetShaderResources(0, 1, &nullSRV);
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

    deviceContext->VSSetConstantBuffers(0, 1, &sharedResources.perFrameBuffer);
    deviceContext->VSSetConstantBuffers(1, 1, &sharedResources.perObjectBuffer);

    deviceContext->PSSetConstantBuffers(0, 1, &sharedResources.perFrameBuffer);
    deviceContext->PSSetConstantBuffers(1, 1, &sharedResources.perMaterialBuffer);
    deviceContext->PSSetConstantBuffers(2, 1, &sharedResources.lightingBuffer);

    ID3D11ShaderResourceView *directionalLightBufferSRV = this->shadowSystem->GetDirectionalLightBufferSRV();
    ID3D11ShaderResourceView *spotLightBufferSRV = this->shadowSystem->GetSpotLightBufferSRV();
    deviceContext->PSSetShaderResources(0, 1, &directionalLightBufferSRV);
    deviceContext->PSSetShaderResources(1, 1, &spotLightBufferSRV);

    for (int i = 0; i < this->perFrameProbeData.count; ++i) {
        for (int face = 0; face < 6; ++face) {
            Render_view *view = context.GetView(View_type::cubeFace, i * 6 + face);
            if (!view)
                continue;

            ID3D11RenderTargetView *rtv = this->probeRTVs[i * 6 + face];

            if (!skyboxSRV)
                deviceContext->ClearRenderTargetView(rtv, clearColour);

            deviceContext->ClearDepthStencilView(this->probeDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
            deviceContext->OMSetRenderTargets(1, &rtv, this->probeDepthDSV);

            sharedResources.UploadPerFrameData(deviceContext, *view);

            for (const Geometry_command &command : view->queue.geometryCommands) {
                if (command.isReflective)
                    continue;

                Per_object_data perObjectData{};
                perObjectData.worldMatrix = command.worldMatrix;
                XMStoreFloat4x4(
                    &perObjectData.worldMatrixInvTranspose, 
                    XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&command.worldMatrix)))
                );

                UploadConstantBuffer(deviceContext, sharedResources.perObjectBuffer, perObjectData);

                Material *material = command.material.Get();
                if (material) {
                    Per_material_data perMaterialData{};
                    perMaterialData.materialDiffuse          = material->diffuseColour;
                    perMaterialData.isReflective             = command.isReflective;
                    perMaterialData.materialSpecular         = material->specularColour;
                    perMaterialData.materialSpecularExponent = material->specularExponent;

                    UploadConstantBuffer(deviceContext, sharedResources.perMaterialBuffer, perMaterialData);

                    if (Texture2D *texture = material->diffuseTexture.Get())
                        deviceContext->PSSetShaderResources(2, 1, &texture->shaderResourceView);
                }

                const UINT stride = sizeof(Vertex);
                const UINT offset = 0;
                deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
            }
        }
    }

    deviceContext->GenerateMips(this->probeSRV);

    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView *nullSRVs[3] = {};
    deviceContext->PSSetShaderResources(0, 3, nullSRVs);
}

bool ReflectionProbeSystem::CreateTextureCubeArray(ID3D11Device *device) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = REFLECTION_PROBE_RESOLUTION;
    desc.Height = REFLECTION_PROBE_RESOLUTION;
    desc.MipLevels = 0;
    desc.ArraySize = MAX_REFLECTION_PROBES * 6;
    desc.Format = DXGI_FORMAT_R11G11B10_FLOAT; // HDR output
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HRESULT result = device->CreateTexture2D(&desc, nullptr, &this->probeTexture);
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

    result = device->CreateShaderResourceView(this->probeTexture, &srvDesc, &this->probeSRV);
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
        result = device->CreateRenderTargetView(this->probeTexture, &rtvDesc, &this->probeRTVs[i]);
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
        result = device->CreateUnorderedAccessView(this->probeTexture, &uavDesc, &this->probeUAVs[i]);
        if (FAILED(result)) {
            LogError("Failed to create UAV[%d]", i);
            return false;
        }
    }

    return true;
}

bool ReflectionProbeSystem::CreateDepthTexture(ID3D11Device *device) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = REFLECTION_PROBE_RESOLUTION;
    desc.Height = REFLECTION_PROBE_RESOLUTION;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    HRESULT result = device->CreateTexture2D(&desc, nullptr, &this->probeDepthTexture);
    if (FAILED(result)) {
        LogError("Failed to create depth stencil texture");
        return false;
    }

    result = device->CreateDepthStencilView(this->probeDepthTexture, nullptr, &this->probeDepthDSV);
    if (FAILED(result)) {
        LogError("Failed to create DSV");
        return false;
    }

    return true;
}

bool ReflectionProbeSystem::LoadShaders(ID3D11Device *device, const std::string &shaderDir) {
    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_reflection.cso", bytecode))
        return false;

    HRESULT result = device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->reflectionVS);
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

    result = device->CreateInputLayout(layoutDesc, 4, bytecode.data(), bytecode.size(), &this->reflectionLayout);
    if (FAILED(result)) {
        LogError("Failed to create input layout");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ps_reflection.cso", bytecode))
        return false;

    result = device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &this->reflectionPS);
    if (FAILED(result)) {
        LogError("Failed to create pixel shader");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "cs_skybox.cso", bytecode))
        return false;

    result = device->CreateComputeShader(bytecode.data(), bytecode.size(), nullptr, &this->skyboxCS);
    if (FAILED(result)) {
        LogError("Failed to create skybox compute shader");
        return false;
    }

    return true;
}

bool ReflectionProbeSystem::Initialize(ID3D11Device *device, const std::string &shaderDir, ShadowSystem *shadowSystem) {
    LogInfo("Creating reflection probe system...\n");

    if (!this->CreateTextureCubeArray(device))
        return false;

    if (!this->CreateDepthTexture(device))
        return false;

    if (!CreateStructuredBuffer(
        device,
        sizeof(Reflection_probe_data),
        MAX_REFLECTION_PROBES,
        &this->probeBuffer,
        &this->probeBufferSRV,
        "reflection"
    )) {
        return false;
    }

    if (!this->LoadShaders(device, shaderDir))
        return false;

    this->shadowSystem = shadowSystem;

    return true;
}

void ReflectionProbeSystem::Shutdown() {
    SafeRelease(this->reflectionVS);
    SafeRelease(this->reflectionPS);
    SafeRelease(this->reflectionLayout);
    SafeRelease(this->skyboxCS);

    SafeRelease(this->probeSRV);
    for (int i = 0; i < MAX_REFLECTION_PROBES * 6; ++i) {
        SafeRelease(this->probeRTVs[i]);
        SafeRelease(this->probeUAVs[i]);
    }
    SafeRelease(this->probeTexture);

    SafeRelease(this->probeDepthDSV);
    SafeRelease(this->probeDepthTexture);

    SafeRelease(this->probeBufferSRV);
    SafeRelease(this->probeBuffer);
}

void ReflectionProbeSystem::PrepareViews(Scene *scene, const Render_view &primaryView, std::vector<Render_view> &outViews) {
    if (!scene) {
        LogWarn("Scene was nullptr\n");
        return;
    }

    this->perFrameProbeData.count = 0;

    const auto &reflectionProbeCommands = primaryView.queue.reflectionProbeCommands;
    if (reflectionProbeCommands.empty())
        return;

    static constexpr struct { XMFLOAT3 forward; XMFLOAT3 up; } faces[6] = {
        {{ 1,  0,  0}, {0,  1,  0}}, // +x
        {{-1,  0,  0}, {0,  1,  0}}, // -x
        {{ 0,  1,  0}, {0,  0, -1}}, // +y
        {{ 0, -1,  0}, {0,  0,  1}}, // -y
        {{ 0,  0,  1}, {0,  1,  0}}, // +z
        {{ 0,  0, -1}, {0,  1,  0}}  // -z
    };

    std::vector<Render_view> views;

    for (int i = 0; i < reflectionProbeCommands.size(); ++i) {
        if (this->perFrameProbeData.count >= MAX_REFLECTION_PROBES)
            break;

        const Reflection_probe_command &command = reflectionProbeCommands[i];
        int slot = this->perFrameProbeData.count;

        XMVECTOR position = XMLoadFloat3(&command.position);
        XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, command.nearPlane, command.farPlane);

        for (int face = 0; face < 6; ++face) {
            XMVECTOR forward = XMLoadFloat3(&faces[face].forward);
            XMVECTOR up = XMLoadFloat3(&faces[face].up);

            XMMATRIX viewMatrix = XMMatrixLookToLH(position, forward, up);

            Render_view view{};
            view.type = View_type::cubeFace;
            view.index = slot * 6 + face;

            XMStoreFloat4x4(&view.viewMatrix, viewMatrix);
            XMStoreFloat4x4(&view.projectionMatrix, projectionMatrix);

            view.cameraPosition = command.position;
            view.nearPlane = command.nearPlane;
            view.farPlane = command.farPlane;

            BoundingFrustum::CreateFromMatrix(view.frustum, projectionMatrix);
            view.frustum.Transform(view.frustum, XMMatrixInverse(nullptr, viewMatrix));

            views.push_back(view);
        }

        this->perFrameProbeData.entries[slot] = {command.position, command.radius};
        ++this->perFrameProbeData.count;
    }

    scene->GatherVisibility(views);
    outViews.insert(outViews.end(), views.begin(), views.end());
}

Reflection_probe_handles ReflectionProbeSystem::RegisterRenderPasses(
    FrameGraph &frameGraph,
    const SharedResources &sharedResources,
    const float *clearColour
) {
    Reflection_probe_handles handles{};

    handles.reflectionProbes = frameGraph.ImportTexture(
        "ReflectionProbe", 
        this->probeTexture, 
        nullptr, 
        this->probeSRV
    );
    handles.reflectionProbeBufferSRV = this->probeBufferSRV;

    struct Reflection_pass_data {
        FrameGraph::TextureHandle reflectionProbes;
    };

    frameGraph.AddRenderPass<Reflection_pass_data>(
        "Reflection pass",
        [&](Reflection_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.reflectionProbes = builder.Write(handles.reflectionProbes);
        },
        [this, &sharedResources, clearColour](const Reflection_pass_data &data, FrameGraph::ExecutionContext &context) {
            this->ExectuteReflectionRenderPass(context, sharedResources, clearColour);
        }
    );

    return handles;
}

void ReflectionProbeSystem::UploadProbeData(ID3D11DeviceContext *deviceContext) const {
    if (this->perFrameProbeData.count <= 0)
        return;

    Reflection_probe_data data[MAX_REFLECTION_PROBES];
    for (int i = 0; i < this->perFrameProbeData.count; ++i) {
        data[i].position = this->perFrameProbeData.entries[i].position;
        data[i].radius = this->perFrameProbeData.entries[i].radius;
        data[i].slotIndex = i;
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result = deviceContext->Map(this->probeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map reflection probe buffer\n");
        return;
    }

    memcpy(mapped.pData, data, sizeof(Reflection_probe_data) * this->perFrameProbeData.count);
    deviceContext->Unmap(this->probeBuffer, 0);
}