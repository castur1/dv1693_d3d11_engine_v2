#ifndef FRAME_GRAPH_HPP
#define FRAME_GRAPH_HPP

#include "rendering/render_queue.hpp"

#include <d3d11.h>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_set>

class FrameGraph {
public:
    typedef uint32_t TextureHandle, PassHandle;
    static constexpr uint32_t INVALID_HANDLE = UINT32_MAX;

    struct Texture_desc {
        enum class Size_mode {
            absolute,
            relative
        };

        Size_mode sizeMode = Size_mode::relative;
        UINT width         = 0;
        UINT height        = 0;
        float widthScale   = 1.0f;
        float heightScale  = 1.0f;
        UINT mipLevels     = 1;
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

        UINT bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    };

    struct Render_pass_base;

    class RenderPassBuilder {
        friend class FrameGraph;

        FrameGraph &frameGraph;
        PassHandle passHandle;

        RenderPassBuilder(FrameGraph &frameGraph, PassHandle passHandle) 
            : frameGraph(frameGraph), passHandle(passHandle) {}

        Render_pass_base *GetRenderPass();

    public:
        TextureHandle Read(TextureHandle textureHandle);
        TextureHandle Write(TextureHandle textureHandle);

        void WritesBackbuffer();
    };

    class ExecutionContext {
        friend class FrameGraph;

        ID3D11DeviceContext *deviceContext;
        const RenderQueue &renderQueue;
        const FrameGraph &frameGraph;

        ExecutionContext(ID3D11DeviceContext *deviceContext, const RenderQueue &renderQueue, const FrameGraph &frameGraph)
            : deviceContext(deviceContext), renderQueue(renderQueue), frameGraph(frameGraph) {}

    public:
        ID3D11DeviceContext *GetDeviceContext() const { return this->deviceContext; }
        const RenderQueue &GetRenderQueue() const { return this->renderQueue; }

        ID3D11ShaderResourceView *GetShaderResourceView(TextureHandle handle) const;
        ID3D11RenderTargetView *GetRenderTargetView(TextureHandle handle) const;
        ID3D11DepthStencilView *GetDepthStencilView(TextureHandle handle) const;
        ID3D11UnorderedAccessView *GetUnorderedAccessView(TextureHandle handle) const;
    };

private:
    struct Texture_resource {
        std::string name;
        Texture_desc desc;
        bool wasImported = false;

        ID3D11Texture2D *texture                       = nullptr;
        ID3D11ShaderResourceView *shaderResourceView   = nullptr;
        ID3D11RenderTargetView *renderTargetView       = nullptr;
        ID3D11DepthStencilView *depthStencilView       = nullptr;
        ID3D11UnorderedAccessView *unorderedAccessView = nullptr;

        PassHandle firstWrite = INVALID_HANDLE;
        PassHandle lastRead   = INVALID_HANDLE;
        int readRefCount    = 0;
    };

    struct Render_pass_base {
        std::string name;

        std::unordered_set<TextureHandle> reads;
        std::unordered_set<TextureHandle> writes;

        bool writesBackbuffer = false;

        bool isCulled = false;
        int refCount = 0;

        virtual ~Render_pass_base() = default;

        virtual void Execute(ExecutionContext &context) = 0;
    };

    template <typename PassData>
    struct Render_pass : Render_pass_base {
        PassData data;
        std::function<void(const PassData &, ExecutionContext &)> executeFunc;

        void Execute(ExecutionContext &context) override {
            this->executeFunc(this->data, context);
        }
    };

    ID3D11Device *device;

    std::vector<Texture_resource> textureResources;

    std::vector<std::unique_ptr<Render_pass_base>> renderPasses;
    std::vector<PassHandle> sortedPassHandles;

    int backbufferWidth;
    int backbufferHeight;

    bool isCompiled;

    void CullUnusedPasses();
    bool TopologicalSort();

    void ComputeResourceLifetimes();
    void CreateTemporaryResources();
    void ReleaseTemporaryResources();

public:
    FrameGraph();
    ~FrameGraph();

    FrameGraph(const FrameGraph &other) = delete;
    FrameGraph &operator=(const FrameGraph &other) = delete;

    void SetDevice(ID3D11Device *device);

    TextureHandle CreateTexture(const std::string &name, Texture_desc desc);

    TextureHandle ImportTexture(
        const std::string &name,
        ID3D11Texture2D *texture,
        ID3D11RenderTargetView *renderTargetView,
        ID3D11ShaderResourceView *shaderResourceView = nullptr,
        ID3D11DepthStencilView *depthStencilView = nullptr,
        ID3D11UnorderedAccessView *unorderedAccessView = nullptr
    );

    void UpdateImportedTexture(
        TextureHandle handle,
        ID3D11Texture2D *texture,
        ID3D11RenderTargetView *renderTargetView,
        ID3D11ShaderResourceView *shaderResourceView = nullptr,
        ID3D11DepthStencilView *depthStencilView = nullptr,
        ID3D11UnorderedAccessView *unorderedAccessView = nullptr
    );

    template <typename PassData>
    PassData &AddRenderPass(
        const std::string &name,
        std::function<void(PassData &, RenderPassBuilder &)> setupFunc,
        std::function<void(const PassData &, ExecutionContext &)> executeFunc
    ) {
        PassHandle handle = this->renderPasses.size();

        Render_pass<PassData> *renderPass = new Render_pass<PassData>();
        renderPass->name = name;

        this->renderPasses.emplace_back(renderPass);

        RenderPassBuilder builder(*this, handle);
        setupFunc(renderPass->data, builder);

        renderPass->executeFunc = std::move(executeFunc);

        return renderPass->data;
    }

    void Compile(int backbufferWidth, int backbufferHeight);
    void OnResize(int width, int height);

    void Execute(ID3D11DeviceContext *deviceContext, const RenderQueue &renderQueue);

    void Clear();

    void LogGraph() const;
};

#endif