#include "pipeline_state_loader.hpp"
#include "core/logging.hpp"
#include "resources/asset_manager.hpp"

Pipeline_state *PipelineStateLoader::Load(const std::string &path) {
    return nullptr;
}

Pipeline_state *PipelineStateLoader::CreateDefault() {
    Pipeline_state *pipelineState = new Pipeline_state();

    if (!this->LoadShaders(
        "shaders/vs_default.cso",
        "shaders/ps_default.cso",
        &pipelineState->vertexShader,
        &pipelineState->pixelShader,
        &pipelineState->inputLayout
    )) {
        delete pipelineState;
        LogWarn("Failed to load default shaders\n");
        return nullptr;
    }

    pipelineState->primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    LogInfo("Default Pipeline_state asset created\n");

    return pipelineState;
}

bool PipelineStateLoader::LoadShaders(
    const std::string &vertexShaderPath,
    const std::string &pixelShaderPath,
    ID3D11VertexShader **vertexShader,
    ID3D11PixelShader **pixelShader,
    ID3D11InputLayout **inputLayout
) {
    std::string assetDir = this->assetManager->GetAssetDirectory();

    void *vertexShaderBuffer{};
    size_t vertexShaderBufferSize{};
    if (!this->assetManager->LoadFileContents(assetDir + vertexShaderPath, &vertexShaderBuffer, &vertexShaderBufferSize))
        return false;

    HRESULT result = device->CreateVertexShader(vertexShaderBuffer, vertexShaderBufferSize, nullptr, vertexShader);
    if (FAILED(result)) {
        free(vertexShaderBuffer);
        LogWarn("Failed to create vertex shader '%s'\n", vertexShaderPath.c_str());
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    result = device->CreateInputLayout(inputElementDescs, 3, vertexShaderBuffer, vertexShaderBufferSize, inputLayout);

    free(vertexShaderBuffer);

    if (FAILED(result)) {
        LogWarn("Failed to create input layout for '%s'\n", vertexShaderPath.c_str());
        (*vertexShader)->Release();
        return false;
    }

    void *pixelShaderBuffer{};
    size_t pixelShaderBufferSize{};
    if (!this->assetManager->LoadFileContents(assetDir + pixelShaderPath, &pixelShaderBuffer, &pixelShaderBufferSize)) {
        (*vertexShader)->Release();
        (*inputLayout)->Release();
        return false;
    }

    result = device->CreatePixelShader(pixelShaderBuffer, pixelShaderBufferSize, nullptr, pixelShader);

    free(pixelShaderBuffer);

    if (FAILED(result)) {
        LogWarn("Failed to create pixel shader '%s'\n", pixelShaderPath.c_str());
        (*vertexShader)->Release();
        (*inputLayout)->Release();
        return false;
    }

    return true;
}