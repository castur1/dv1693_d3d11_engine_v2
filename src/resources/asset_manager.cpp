#include "asset_manager.hpp"
#include "core/logging.hpp"

#include "tiny_obj_loader/tiny_obj_loader.h"
#include "stb_image/stb_image.h"

#include <fstream>

AssetManager::AssetManager() : device(nullptr), assetDir("assets/") {}

AssetManager::~AssetManager() {}

bool AssetManager::Initialize(ID3D11Device *device) {
    LogInfo("Creating asset manager...\n");
    LogIndent();

    this->device = device;

    this->assetRegistry.SetAssetDirectory(this->assetDir);
    this->assetRegistry.RegisterAssets();

    this->CreateDefaultAssets();

    LogUnindent();

    return true;
}

void AssetManager::CreateDefaultAssets() {
    this->pipelineStateCache.SetDefaultAsset(this->CreateDefaultPipelineState());
    this->texture2DCache.SetDefaultAsset(this->CreateDefaultTexture2D());
    this->materialCache.SetDefaultAsset(this->CreateDefaultMaterial());
    this->modelCache.SetDefaultAsset(this->CreateDefaultModel());
}

Model *AssetManager::CreateDefaultModel() {
    Model *model = new Model();

    LogInfo("Created default Model\n");

    return model;
}

Texture2D *AssetManager::CreateDefaultTexture2D() {
    const UINT width  = 1;
    const UINT height = 1;
    UINT32 pixels[width * height] = {
        0xFFFFFFFF
    };

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA inititialData{};
    inititialData.pSysMem = pixels;
    inititialData.SysMemPitch = width * sizeof(UINT);
    inititialData.SysMemSlicePitch = 0;

    ID3D11Texture2D *texture{};
    HRESULT result = this->device->CreateTexture2D(&textureDesc, &inititialData, &texture);
    if (FAILED(result)) {
        LogWarn("Failed to create default Texture2D\n");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    Texture2D *texture2D = new Texture2D();

    result = this->device->CreateShaderResourceView(texture, &shaderResourceViewDesc, 
        &texture2D->shaderResourceView);

    texture->Release();

    if (FAILED(result)) {
        delete texture2D;
        LogWarn("Failed to create default Texture2D SRV\n");
        return nullptr;
    }

    LogInfo("Created default Texture2D\n");

    return texture2D;
}

Material *AssetManager::CreateDefaultMaterial() {
    Material *material = new Material();

    material->pipelineState = this->pipelineStateCache.GetDefaultAsset();
    material->diffuseTexture = this->texture2DCache.GetDefaultAsset();
    material->ambientColour = {1.0f, 1.0f, 1.0f};
    material->diffuseColour = {1.0f, 1.0f, 1.0f};
    material->specularColour = {1.0f, 1.0f, 1.0f};
    material->specularExponent = 32.0f;

    LogInfo("Created default Material\n");

    return material;
}

Pipeline_state *AssetManager::CreateDefaultPipelineState() {
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

    LogInfo("Created default Pipeline_state\n");

    return pipelineState;
}

bool AssetManager::LoadFileContents(const std::string &path, void **buffer, size_t *size) {
    if (!buffer || !size)
        return false;

    std::ifstream file(this->assetDir + path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        LogWarn("Failed to open file '%s'\n", path.c_str());
        return false;
    }

    *size = file.tellg();
    *buffer = malloc(*size);

    if (!*buffer) {
        LogWarn("Failed to allocate memory for file '%s'\n", path.c_str());
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read((char *)*buffer, *size);
    file.close();

    return true;
}

bool AssetManager::LoadShaders(
    const std::string &vertexShaderPath,
    const std::string &pixelShaderPath,
    ID3D11VertexShader **vertexShader, 
    ID3D11PixelShader **pixelShader, 
    ID3D11InputLayout **inputLayout
) {
    void *vertexShaderBuffer{};
    size_t vertexShaderBufferSize{};

    if (!this->LoadFileContents(vertexShaderPath, &vertexShaderBuffer, &vertexShaderBufferSize))
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

    if (!this->LoadFileContents(pixelShaderPath, &pixelShaderBuffer, &pixelShaderBufferSize)) {
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

void AssetManager::MarkAllAssetsUnused() {
    this->modelCache.MarkAllUnused();
    this->texture2DCache.MarkAllUnused();
    this->materialCache.MarkAllUnused();
    this->pipelineStateCache.MarkAllUnused();
}

void AssetManager::MarkAsUsed(AssetID uuid) {
    this->modelCache.MarkAsUsed(uuid);
    this->texture2DCache.MarkAsUsed(uuid);
    this->materialCache.MarkAsUsed(uuid);
    this->pipelineStateCache.MarkAsUsed(uuid);
}

void AssetManager::CleanUpUnused() {
    this->modelCache.CleanUpUnused();
    this->texture2DCache.CleanUpUnused();
    this->materialCache.CleanUpUnused();
    this->pipelineStateCache.CleanUpUnused();
}

void AssetManager::SetAssetDirectory(const std::string &path) {
    this->assetDir = path;
}