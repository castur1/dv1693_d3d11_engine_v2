#ifndef SHARED_RESOURCES_HPP
#define SHARED_RESOURCES_HPP

#include "rendering/render_data.hpp"
#include "rendering/render_utils.hpp"
#include "rendering/render_view.hpp"

#include <d3d11.h>

enum class Sampler_slot : int {
    linearWrap,
    shadowComparison,

    count
};

constexpr int operator+(Sampler_slot slot) noexcept {
    return static_cast<int>(slot);
}

class SharedResources {
    friend class Renderer;

public:
    ID3D11Buffer *perObjectBuffer = nullptr;
    ID3D11Buffer *perFrameBuffer = nullptr;
    ID3D11Buffer *perMaterialBuffer = nullptr;
    ID3D11Buffer *lightingBuffer = nullptr;

    ID3D11SamplerState *samplers[+Sampler_slot::count] = {};

private:
    bool CreateConstantBuffers(ID3D11Device *device);
    bool CreateSamplers(ID3D11Device *device);

    bool Initialize(ID3D11Device *device);
    void Shutdown();

    SharedResources() = default;

public:
    void BindSamplers(ID3D11DeviceContext *deviceContext) const;
    void UploadPerFrameData(ID3D11DeviceContext *deviceContext, const Render_view &view) const;
};

#endif