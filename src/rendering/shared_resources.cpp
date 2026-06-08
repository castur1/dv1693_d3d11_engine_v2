#include "shared_resources.hpp"

bool SharedResources::CreateConstantBuffers(ID3D11Device *device) {
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    bufferDesc.ByteWidth = sizeof(Per_object_data);
    HRESULT result = device->CreateBuffer(&bufferDesc, nullptr, &this->perObjectBuffer);
    if (FAILED(result)) {
        LogError("Failed to create per-object constant buffer");
        return false;
    }

    bufferDesc.ByteWidth = sizeof(Per_frame_data);
    result = device->CreateBuffer(&bufferDesc, nullptr, &this->perFrameBuffer);
    if (FAILED(result)) {
        LogError("Failed to create per-frame constant buffer");
        return false;
    }

    bufferDesc.ByteWidth = sizeof(Per_material_data);
    result = device->CreateBuffer(&bufferDesc, nullptr, &this->perMaterialBuffer);
    if (FAILED(result)) {
        LogError("Failed to create per-material constant buffer");
        return false;
    }

    bufferDesc.ByteWidth = sizeof(Lighting_data);
    result = device->CreateBuffer(&bufferDesc, nullptr, &this->lightingBuffer);
    if (FAILED(result)) {
        LogError("Failed to create lighting constant buffer");
        return false;
    }

    LogInfo("Constant buffers created\n");

    return true;
}

bool SharedResources::CreateSamplers(ID3D11Device *device) {
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        HRESULT result = device->CreateSamplerState(&desc, &this->samplers[+Sampler_slot::linearWrap]);
        if (FAILED(result)) {
            LogError("Failed to create linear wrap sampler");
            return false;
        }
    }

    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = 1.0f;
        desc.BorderColor[3] = 1.0f;
        desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        HRESULT result = device->CreateSamplerState(&desc, &this->samplers[+Sampler_slot::shadowComparison]);
        if (FAILED(result)) {
            LogError("Failed to create shadow comparison sampler");
            return false;
        }
    }

    LogInfo("Sampler states created\n");

    return true;
}

bool SharedResources::Initialize(ID3D11Device *device) {
    LogInfo("Creating shared rendering resources...\n");
    LogIndent();

    if (!this->CreateConstantBuffers(device))
        return false;

    if (!this->CreateSamplers(device))
        return false;

    LogUnindent();
    return true;
}

void SharedResources::Shutdown() {
    SafeRelease(this->perFrameBuffer);
    SafeRelease(this->perObjectBuffer);
    SafeRelease(this->perMaterialBuffer);
    SafeRelease(this->lightingBuffer);

    for (int i = 0; i < +Sampler_slot::count; ++i)
        SafeRelease(this->samplers[i]);
}

void SharedResources::BindSamplers(ID3D11DeviceContext *deviceContext) const {
    deviceContext->PSSetSamplers(0, +Sampler_slot::count, this->samplers);
    deviceContext->CSSetSamplers(0, +Sampler_slot::count, this->samplers);
    deviceContext->DSSetSamplers(0, +Sampler_slot::count, this->samplers);
}

void SharedResources::UploadPerFrameData(ID3D11DeviceContext *deviceContext, const Render_view &view) const {
    XMMATRIX viewMatrix = XMLoadFloat4x4(&view.viewMatrix);
    XMMATRIX projectionMatrix = XMLoadFloat4x4(&view.projectionMatrix);
    XMMATRIX viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);

    Per_frame_data data{};
    XMStoreFloat4x4(&data.viewMatrix, XMMatrixTranspose(viewMatrix));
    XMStoreFloat4x4(&data.invViewMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, viewMatrix)));
    XMStoreFloat4x4(&data.projectionMatrix, XMMatrixTranspose(projectionMatrix));
    XMStoreFloat4x4(&data.invProjectionMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, projectionMatrix)));
    XMStoreFloat4x4(&data.viewProjectionMatrix, XMMatrixTranspose(viewProjectionMatrix));
    XMStoreFloat4x4(&data.invViewProjectionMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, viewProjectionMatrix)));
    data.cameraPosition = view.cameraPosition;

    UploadConstantBuffer(deviceContext, this->perFrameBuffer, data);
}
