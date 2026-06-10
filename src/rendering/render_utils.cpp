#include "render_utils.hpp"

#include <fstream>

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

bool LoadShaderBytecode(const std::string &path, std::vector<uint8_t> &outBytecode) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LogError("Failed to load file '%s'", path.c_str());
        return false;
    }

    const std::streampos fileSize = file.tellg();
    if (fileSize <= 0) {
        LogError("File '%s' is empty or invalid", path.c_str());
        return false;
    }

    const size_t size = static_cast<size_t>(fileSize);
    outBytecode.resize(size);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(outBytecode.data()), size);

    return true;
}

bool CreateDepthStencilArray(
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

bool CreateStructuredBuffer(
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

Render_view *GetView(std::vector<Render_view> &views, View_type type, int index) {
    for (Render_view &view : views)
        if (view.type == type && view.index == index)
            return &view;

    return nullptr;
}