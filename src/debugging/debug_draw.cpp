#include "debug_draw.hpp"
#include "core/logging.hpp"

#include <fstream>

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

ID3D11Device *DebugDraw::device = nullptr;

ID3D11Buffer *DebugDraw::vertexBuffer     = nullptr;
ID3D11InputLayout *DebugDraw::inputLayout = nullptr;

ID3D11VertexShader *DebugDraw::vertexShader = nullptr;
ID3D11PixelShader *DebugDraw::pixelShader   = nullptr;

ID3D11Buffer *DebugDraw::constantBuffer = nullptr;

const UINT DebugDraw::MAX_VERTEX_COUNT = 16384;
std::vector<Debug_vertex> DebugDraw::vertices;

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

bool DebugDraw::LoadShaders(ID3D11Device *device) {
    const std::string shaderDir = "assets/shaders/";

    std::vector<uint8_t> bytecode;

    if (!LoadShaderBytecode(shaderDir + "vs_debug.cso", bytecode))
        return false;

    HRESULT result = device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, &vertexShader);
    if (FAILED(result)) {
        LogError("Failed to create vertex shader");
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOUR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    result = device->CreateInputLayout(layoutDesc, 2, bytecode.data(), bytecode.size(), &inputLayout);
    if (FAILED(result)) {
        LogError("Failed to create input layout");
        return false;
    }

    if (!LoadShaderBytecode(shaderDir + "ps_debug.cso", bytecode))
        return false;

    result = device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, &pixelShader);
    if (FAILED(result)) {
        LogError("Failed to create pixel shader");
        return false;
    }

    return true;
}

bool DebugDraw::Initialize(ID3D11Device *device) {
    LogInfo("Creating debug renderer...\n");

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = sizeof(Debug_vertex) * MAX_VERTEX_COUNT;
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT result = device->CreateBuffer(&vertexBufferDesc, nullptr, &vertexBuffer);
    if (FAILED(result)) {
        LogError("Failed to create vertex buffer");
        return false;
    }

    D3D11_BUFFER_DESC constantBufferDesc{};
    constantBufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    result = device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
    if (FAILED(result)) {
        LogError("Failed to create constant buffer");
        return false;
    }

    if (!LoadShaders(device))
        return false;

    return true;
}

void DebugDraw::Shutdown() {
    SafeRelease(constantBuffer);
    SafeRelease(pixelShader);
    SafeRelease(vertexShader);
    SafeRelease(inputLayout);
    SafeRelease(vertexBuffer);
}

void DebugDraw::Render(ID3D11DeviceContext *deviceContext, const XMFLOAT4X4 &viewProjectionMatrix) {
    if (vertices.empty())
        return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result = deviceContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map vertex buffer\n");
        return;
    }

    memcpy(mapped.pData, vertices.data(), sizeof(Debug_vertex) * vertices.size());
    deviceContext->Unmap(vertexBuffer, 0);

    result = deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map constant buffer\n");
        return;
    }

    *(XMFLOAT4X4 *)mapped.pData = viewProjectionMatrix;
    deviceContext->Unmap(constantBuffer, 0);

    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

    UINT stride = sizeof(Debug_vertex);
    UINT offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    deviceContext->Draw(vertices.size(), 0);

    deviceContext->VSSetShader(nullptr, nullptr, 0);
    deviceContext->PSSetShader(nullptr, nullptr, 0);

    vertices.clear();
}

void DebugDraw::Line(const XMFLOAT3 &v1, const XMFLOAT3 &v2, const XMFLOAT4 &colour) {
    if (vertices.size() + 2 > MAX_VERTEX_COUNT) {
        LogWarn("Max vertex count exceeded\n");
        return;
    }

    vertices.push_back({v1, colour});
    vertices.push_back({v2, colour});
}

void DebugDraw::Box(const BoundingBox &box, const XMFLOAT4 &colour) {
    XMFLOAT3 corners[8];
    box.GetCorners(corners);

    Line(corners[0], corners[1], colour);
    Line(corners[1], corners[2], colour);
    Line(corners[2], corners[3], colour);
    Line(corners[3], corners[0], colour);

    Line(corners[4], corners[5], colour);
    Line(corners[5], corners[6], colour);
    Line(corners[6], corners[7], colour);
    Line(corners[7], corners[4], colour);

    Line(corners[0], corners[4], colour);
    Line(corners[1], corners[5], colour);
    Line(corners[2], corners[6], colour);
    Line(corners[3], corners[7], colour);
}

void DebugDraw::Frustum(const BoundingFrustum &frustum, const XMFLOAT4 &colour) {
    XMFLOAT3 corners[8];
    frustum.GetCorners(corners);

    Line(corners[0], corners[1], colour);
    Line(corners[1], corners[2], colour);
    Line(corners[2], corners[3], colour);
    Line(corners[3], corners[0], colour);

    Line(corners[4], corners[5], colour);
    Line(corners[5], corners[6], colour);
    Line(corners[6], corners[7], colour);
    Line(corners[7], corners[4], colour);

    Line(corners[0], corners[4], colour);
    Line(corners[1], corners[5], colour);
    Line(corners[2], corners[6], colour);
    Line(corners[3], corners[7], colour);
}

XMFLOAT4 DebugDraw::VectorToFloat4(const XMVECTOR &v) {
    XMFLOAT4 output;
    XMStoreFloat4(&output, v);
    return output;
}
