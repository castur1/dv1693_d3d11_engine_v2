#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

struct Draw_command {

};

enum class Sampler_state_type {
    LINEAR_WRAP = 0,
    COUNT
};

class Renderer {
    ID3D11Device *device;
    ID3D11DeviceContext *deviceContext;
    IDXGISwapChain *swapChain;
    ID3D11RenderTargetView *renderTargetView;
    ID3D11Texture2D *depthStencilTexture;
    ID3D11DepthStencilView *depthStencilView;
    ID3D11SamplerState *samplerStates[(int)Sampler_state_type::COUNT];
    D3D11_VIEWPORT viewport;

    int width;
    int height;

    float clearColour[4];

    bool CreateInterface(HWND hWnd);
    bool CreateRenderTargetView();
    bool CreateDepthStencil(int width, int height);
    bool CreateCommonSamplerStates();
    void SetViewport(int width, int height);

    void BindCommonSamplerStates();

public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hWnd);
    bool Resize(int width, int height);

    // void Submit();
    // void Flush();
    void Begin();
    void End();

    ID3D11Device *GetDevice() const;
    int GetWidth() const;
    int GetHeight() const;
    float GetAspectRatio() const;

    void ToggleFullscreen();
    bool IsFullscreened();

    void SetClearColour(float r, float g, float b, float a = 1.0f);
    const float *GetClearColour() const;
};

#endif