#include "shadow_system.hpp"
#include "core/logging.hpp"
#include "rendering/render_utils.hpp"
#include "scene/scene.hpp"

#include <algorithm>

#undef min
#undef max

void ShadowSystem::ComputeDirectionalLightMatrices(
    XMMATRIX &outView,
    XMMATRIX &outProjection,
    const Directional_light_command &command,
    const Render_view &primaryView
) const {
    XMFLOAT3 corners[8];
    primaryView.frustum.GetCorners(corners);

    float t = std::min(primaryView.shadowDistance / primaryView.farPlane, 1.0f);
    for (int i = 0; i < 4; ++i)
        XMStoreFloat3(&corners[i + 4], XMVectorLerp(XMLoadFloat3(&corners[i]), XMLoadFloat3(&corners[i + 4]), t));

    XMVECTOR centre = XMVectorZero();
    for (const XMFLOAT3 &corner : corners)
        centre += XMLoadFloat3(&corner);
    centre /= 8.0f;

    float radius = 0.0f;
    for (const XMFLOAT3 &corner : corners) {
        float distance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&corner) - centre));
        radius = std::max(radius, distance);
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
        minZ = std::min(minZ, z);
        maxZ = std::max(maxZ, z);
    }
    minZ -= 500.0f;

    outProjection = XMMatrixOrthographicOffCenterLH(-radius, radius, -radius, radius, minZ, maxZ);
}

void ShadowSystem::ComputeSpotLightMatrices(
    XMMATRIX &outView,
    XMMATRIX &outProjection,
    const Spot_light_command &command
) const {
    XMVECTOR position = XMLoadFloat3(&command.position);
    XMVECTOR direction = XMVector3Normalize(XMLoadFloat3(&command.direction));
    XMVECTOR up = fabsf(XMVectorGetY(direction)) < 0.99f ? XMVectorSet(0, 1, 0, 0) : XMVectorSet(1, 0, 0, 0);

    outView = XMMatrixLookToLH(position, direction, up);
    outProjection = XMMatrixPerspectiveFovLH(command.outerConeAngle * 2.0f, 1.0f, 0.1f, command.range);
}

void ShadowSystem::ExecuteShadowPass(
    FrameGraph::ExecutionContext &context,
    const SharedResources &sharedResources,
    FrameGraph::TextureHandle directionalHandle,
    FrameGraph::TextureHandle spotHandle
) {
    ID3D11DeviceContext *deviceContext = context.GetDeviceContext();

    deviceContext->RSSetState(this->shadowRS);

    deviceContext->IASetInputLayout(this->shadowLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    deviceContext->VSSetShader(this->shadowVS, nullptr, 0);
    deviceContext->PSSetShader(this->shadowPS, nullptr, 0);

    deviceContext->VSSetConstantBuffers(0, 1, &this->shadowBuffer);
    deviceContext->VSSetConstantBuffers(1, 1, &sharedResources.perObjectBuffer);

    {
        D3D11_VIEWPORT viewport{};
        viewport.Width = viewport.Height = SHADOW_MAP_DIRECTIONAL_RESOLUTION;
        viewport.MaxDepth = 1.0f;
        deviceContext->RSSetViewports(1, &viewport);

        for (int i = 0; i < this->perFrameShadowData.directionalCount; ++i) {
            Render_view *view = context.GetView(View_type::shadowMapDirectional, i);
            if (!view)
                break;

            deviceContext->ClearDepthStencilView(this->shadowMapDirectionalDSVs[i], D3D11_CLEAR_DEPTH, 1.0f, 0);

            deviceContext->OMSetRenderTargets(0, nullptr, this->shadowMapDirectionalDSVs[i]);

            XMMATRIX viewMatrix = XMLoadFloat4x4(&view->viewMatrix);
            XMMATRIX projectionMatrix = XMLoadFloat4x4(&view->projectionMatrix);
            XMFLOAT4X4 viewProjectionMatrix;
            XMStoreFloat4x4(&viewProjectionMatrix, XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix)));
            UploadConstantBuffer(deviceContext, this->shadowBuffer, viewProjectionMatrix);

            for (const Geometry_command &command : view->queue.geometryCommands) {
                Per_object_data perObjectData{};
                perObjectData.worldMatrix = command.worldMatrix;
                UploadConstantBuffer(deviceContext, sharedResources.perObjectBuffer, perObjectData);

                Material *material = command.material.Get();
                if (material) {
                    deviceContext->PSSetShaderResources(0, 1, &material->diffuseTexture.Get()->shaderResourceView); // Required for alpha testing
                }

                const UINT stride = sizeof(Vertex);
                const UINT offset = 0;
                deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
            }
        }
    }

    {
        D3D11_VIEWPORT viewport{};
        viewport.Width = viewport.Height = SHADOW_MAP_SPOT_RESOLUTION;
        viewport.MaxDepth = 1.0f;
        deviceContext->RSSetViewports(1, &viewport);

        for (int i = 0; i < this->perFrameShadowData.spotCount; ++i) {
            Render_view *view = context.GetView(View_type::shadowMapSpot, i);
            if (!view)
                break;

            deviceContext->ClearDepthStencilView(this->shadowMapSpotDSVs[i], D3D11_CLEAR_DEPTH, 1.0f, 0);

            deviceContext->OMSetRenderTargets(0, nullptr, this->shadowMapSpotDSVs[i]);

            XMMATRIX viewMatrix = XMLoadFloat4x4(&view->viewMatrix);
            XMMATRIX projectionMatrix = XMLoadFloat4x4(&view->projectionMatrix);
            XMFLOAT4X4 viewProjectionMatrix;
            XMStoreFloat4x4(&viewProjectionMatrix, XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix)));
            UploadConstantBuffer(deviceContext, this->shadowBuffer, viewProjectionMatrix);

            for (const Geometry_command &command : view->queue.geometryCommands) {
                Per_object_data perObjectData{};
                perObjectData.worldMatrix = command.worldMatrix;
                UploadConstantBuffer(deviceContext, sharedResources.perObjectBuffer, perObjectData);

                Material *material = command.material.Get();
                if (material)
                    deviceContext->PSSetShaderResources(0, 1, &material->diffuseTexture.Get()->shaderResourceView); // Required for alpha testing

                const UINT stride = sizeof(Vertex);
                const UINT offset = 0;
                deviceContext->IASetVertexBuffers(0, 1, &command.vertexBuffer, &stride, &offset);
                deviceContext->IASetIndexBuffer(command.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                deviceContext->DrawIndexed(command.indexCount, command.startIndex, command.baseVertex);
            }
        }
    }

    deviceContext->RSSetState(nullptr);
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

bool ShadowSystem::CreateConstantBuffers(ID3D11Device *device) {
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(XMFLOAT4X4);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT result = device->CreateBuffer(&desc, nullptr, &this->shadowBuffer);
    if (FAILED(result)) {
        LogError("Failed to create constant buffer");
        return false;
    }

    return true;
}

bool ShadowSystem::LoadShaders(ID3D11Device *device, const std::string &shaderDir) {
    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_shadow.cso", bytecode))
        return false;

    HRESULT result = device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &this->shadowVS);
    if (FAILED(result)) {
        LogError("Failed to create vertex shader");
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    result = device->CreateInputLayout(layoutDesc, 2, bytecode.data(), bytecode.size(), &this->shadowLayout);
    if (FAILED(result)) {
        LogError("Failed to create input layout");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ps_shadow.cso", bytecode))
        return false;

    result = device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &this->shadowPS);
    if (FAILED(result)) {
        LogError("Failed to create pixel shader");
        return false;
    }

    return true;
}

bool ShadowSystem::CreateRasterizerState(ID3D11Device *device) {
    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthBias = 1000;
    rasterizerDesc.SlopeScaledDepthBias = 1.0f;
    rasterizerDesc.DepthBiasClamp = 0.01f;
    rasterizerDesc.DepthClipEnable = true;

    HRESULT result = device->CreateRasterizerState(&rasterizerDesc, &this->shadowRS);
    if (FAILED(result)) {
        LogError("Failed to create rasterizer state");
        return false;
    }

    return true;
}

bool ShadowSystem::Initialize(ID3D11Device *device, const std::string &shaderDir) {
    LogInfo("Creating shadow system...\n");

    if (!CreateDepthStencilArray(
        device,
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
        device,
        sizeof(Directional_light_data),
        MAX_DIRECTIONAL_LIGHTS,
        &this->directionalLightBuffer,
        &this->directionalLightBufferSRV,
        "directional"
    )) {
        return false;
    }

    if (!CreateDepthStencilArray(
        device,
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
        device,
        sizeof(Spot_light_data),
        MAX_SPOT_LIGHTS,
        &this->spotLightBuffer,
        &this->spotLightBufferSRV,
        "spot"
    )) {
        return false;
    }

    if (!this->CreateConstantBuffers(device))
        return false;

    if (!this->LoadShaders(device, shaderDir))
        return false;

    if (!this->CreateRasterizerState(device))
        return false;

    return true;
}

void ShadowSystem::Shutdown() {
    SafeRelease(this->shadowVS);
    SafeRelease(this->shadowPS);
    SafeRelease(this->shadowLayout);
    SafeRelease(this->shadowRS);
    SafeRelease(this->shadowBuffer);

    SafeRelease(this->directionalLightBufferSRV);
    SafeRelease(this->directionalLightBuffer);
    SafeRelease(this->spotLightBufferSRV);
    SafeRelease(this->spotLightBuffer);

    SafeRelease(this->shadowMapDirectionalSRV);
    for (int i = 0; i < MAX_DIRECTIONAL_SHADOW_MAPS; ++i)
        SafeRelease(this->shadowMapDirectionalDSVs[i]);
    SafeRelease(this->shadowMapDirectionalTexture);

    SafeRelease(this->shadowMapSpotSRV);
    for (int i = 0; i < MAX_SPOT_SHADOW_MAPS; ++i)
        SafeRelease(this->shadowMapSpotDSVs[i]);
    SafeRelease(this->shadowMapSpotTexture);
}

void ShadowSystem::PrepareViews(Scene *scene, const Render_view &primaryView, std::vector<Render_view> &outViews) {
    if (!scene) {
        LogWarn("Scene was nullptr\n");
        return;
    }

    this->perFrameShadowData.directionalCount = 0;
    this->perFrameShadowData.spotCount = 0;

    std::vector<Render_view> views;

    for (int i = 0; i < primaryView.queue.directionalLightCommands.size(); ++i) {
        if (this->perFrameShadowData.directionalCount >= MAX_DIRECTIONAL_SHADOW_MAPS)
            break;

        const Directional_light_command &command = primaryView.queue.directionalLightCommands[i];
        if (!command.castsShadows)
            continue;

        int slot = this->perFrameShadowData.directionalCount;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        this->ComputeDirectionalLightMatrices(viewMatrix, projectionMatrix, command, primaryView);

        Render_view view{};
        view.type = View_type::shadowMapDirectional;
        view.index = slot;
        view.skipFrustumCulling = true;
        XMStoreFloat4x4(&view.viewMatrix, viewMatrix);
        XMStoreFloat4x4(&view.projectionMatrix, projectionMatrix);
        views.push_back(view);

        XMMATRIX viewProjectionMatrix = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix));
        XMStoreFloat4x4(
            &this->perFrameShadowData.directionalViewProjectionMatrices[slot], 
            viewProjectionMatrix
        );
        this->perFrameShadowData.directionalSlotToCommand[slot] = i;
        ++this->perFrameShadowData.directionalCount;
    }

    for (int i = 0; i < primaryView.queue.spotLightCommands.size(); ++i) {
        if (this->perFrameShadowData.spotCount >= MAX_SPOT_SHADOW_MAPS)
            break;

        const Spot_light_command &command = primaryView.queue.spotLightCommands[i];
        if (!command.castsShadows)
            continue;

        int slot = this->perFrameShadowData.spotCount;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        this->ComputeSpotLightMatrices(viewMatrix, projectionMatrix, command);

        Render_view view{};
        view.type = View_type::shadowMapSpot;
        view.index = slot;
        XMStoreFloat4x4(&view.viewMatrix, viewMatrix);
        XMStoreFloat4x4(&view.projectionMatrix, projectionMatrix);

        BoundingFrustum::CreateFromMatrix(view.frustum, projectionMatrix);
        view.frustum.Transform(view.frustum, XMMatrixInverse(nullptr, viewMatrix));

        views.push_back(view);

        XMMATRIX viewProjectionMatrix = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix));
        XMStoreFloat4x4(
            &this->perFrameShadowData.spotViewProjectionMatrices[slot], 
            viewProjectionMatrix
        );
        this->perFrameShadowData.spotSlotToCommand[slot] = i;
        ++this->perFrameShadowData.spotCount;
    }

    scene->GatherVisibility(views);
    outViews.insert(outViews.end(), views.begin(), views.end());
}

Shadow_handles ShadowSystem::RegisterRenderPasses(FrameGraph &frameGraph, const SharedResources &sharedResources) {
    Shadow_handles handles{};

    handles.shadowMapDirectional = frameGraph.ImportTexture(
        "ShadowMap_directional",
        this->shadowMapDirectionalTexture,
        nullptr,
        this->shadowMapDirectionalSRV,
        this->shadowMapDirectionalDSVs[0]
    );

    handles.shadowMapSpot = frameGraph.ImportTexture(
        "ShadowMap_spot",
        this->shadowMapSpotTexture,
        nullptr,
        this->shadowMapSpotSRV,
        this->shadowMapSpotDSVs[0]
    );

    handles.directionalLightBufferSRV = this->directionalLightBufferSRV;
    handles.spotLightBufferSRV = this->spotLightBufferSRV;

    struct Shadow_pass_data {
        FrameGraph::TextureHandle shadowMapDirectional;
        FrameGraph::TextureHandle shadowMapSpot;
    };

    frameGraph.AddRenderPass<Shadow_pass_data>(
        "Shadow pass",
        [&](Shadow_pass_data &data, FrameGraph::RenderPassBuilder &builder) {
            data.shadowMapDirectional = builder.Write(handles.shadowMapDirectional);
            data.shadowMapSpot        = builder.Write(handles.shadowMapSpot);
        },
        [this, &sharedResources](const Shadow_pass_data &data, FrameGraph::ExecutionContext &context) {
            this->ExecuteShadowPass(context, sharedResources, data.shadowMapDirectional, data.shadowMapSpot);
        }
    );

    return handles;
}

void ShadowSystem::UploadLightData(
    ID3D11DeviceContext *deviceContext,
    const Render_view &primaryView,
    const SharedResources &sharedResources,
    int reflectionProbeCount
) {
    Lighting_data lightingData{};
    lightingData.directionalLightCount = primaryView.queue.directionalLightCommands.size();
    lightingData.spotLightCount = primaryView.queue.spotLightCommands.size();
    lightingData.reflectionProbeCount = reflectionProbeCount;
    lightingData.hasSkybox = primaryView.queue.skyboxCommand.has_value() ? 1 : 0;

    for (const auto &dlc : primaryView.queue.directionalLightCommands) {
        lightingData.ambientColour.x += dlc.ambientColour.x;
        lightingData.ambientColour.y += dlc.ambientColour.y;
        lightingData.ambientColour.z += dlc.ambientColour.z;
    }

    UploadConstantBuffer(deviceContext, sharedResources.lightingBuffer, lightingData);

    // Directional

    int directionalCommandToSlot[MAX_DIRECTIONAL_LIGHTS];
    memset(directionalCommandToSlot, -1, sizeof(directionalCommandToSlot));
    for (int i = 0; i < this->perFrameShadowData.directionalCount; ++i)
        directionalCommandToSlot[this->perFrameShadowData.directionalSlotToCommand[i]] = i;

    int directionalCount = std::min((size_t)MAX_DIRECTIONAL_LIGHTS, primaryView.queue.directionalLightCommands.size());

    Directional_light_data directionalData[MAX_DIRECTIONAL_LIGHTS];
    for (int i = 0; i < directionalCount; ++i) {
        const Directional_light_command &dlc = primaryView.queue.directionalLightCommands[i];
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
    HRESULT result = deviceContext->Map(this->directionalLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map directional light buffer\n");
        return;
    }

    memcpy(mapped.pData, directionalData, sizeof(Directional_light_data) * directionalCount);
    deviceContext->Unmap(this->directionalLightBuffer, 0);

    // Spot

    int spotCommandToSlot[MAX_SPOT_LIGHTS];
    memset(spotCommandToSlot, -1, sizeof(spotCommandToSlot));
    for (int i = 0; i < this->perFrameShadowData.spotCount; ++i)
        spotCommandToSlot[this->perFrameShadowData.spotSlotToCommand[i]] = i;

    int spotCount = std::min((size_t)MAX_SPOT_LIGHTS, primaryView.queue.spotLightCommands.size());

    Spot_light_data spotData[MAX_SPOT_LIGHTS];
    for (int i = 0; i < spotCount; ++i) {
        const Spot_light_command &slc = primaryView.queue.spotLightCommands[i];
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

    result = deviceContext->Map(this->spotLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map spot light buffer\n");
        return;
    }

    memcpy(mapped.pData, spotData, sizeof(Spot_light_data) * spotCount);
    deviceContext->Unmap(this->spotLightBuffer, 0);
}