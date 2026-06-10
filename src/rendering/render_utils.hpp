#ifndef RENDER_UTILS_HPP
#define RENDER_UTILS_HPP

#include "core/logging.hpp"
#include "rendering/render_view.hpp"

#include <d3d11.h>
#include <string>
#include <vector>

template <typename T>
inline void UploadConstantBuffer(ID3D11DeviceContext *deviceContext, ID3D11Buffer *buffer, const T &data) {
    if (!deviceContext) {
        LogWarn("Device context was nullptr\n");
        return;
    }

    if (!buffer) {
        LogWarn("Buffer was nullptr\n");
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result = deviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        LogWarn("Failed to map resouce\n");
        return;
    }

    *reinterpret_cast<T *>(mapped.pData) = data;
    deviceContext->Unmap(buffer, 0);
}

bool LoadShaderBytecode(const std::string &path, std::vector<uint8_t> &out);

bool CreateDepthStencilArray(
    ID3D11Device *device,
    int resolution,
    int arraySize,
    ID3D11Texture2D **outTexture,
    ID3D11DepthStencilView **outDSVs,
    ID3D11ShaderResourceView **outSRV,
    const char *debugName
);

bool CreateStructuredBuffer(
    ID3D11Device *device,
    UINT elementSize,
    UINT elementCount,
    ID3D11Buffer **outBuffer,
    ID3D11ShaderResourceView **outSRV,
    const char *debugName
);

Render_view *GetView(std::vector<Render_view> &views, View_type type, int index = 0);

#endif