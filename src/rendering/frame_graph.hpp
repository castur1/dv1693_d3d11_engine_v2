#ifndef FRAME_GRAPH_HPP
#define FRAME_GRAPH_HPP

#include "rendering/render_queue.hpp"

#include <d3d11.h>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

class FrameGraph {
public:
    struct Texture_handle {
        static const uint32_t invalidIndex = UINT32_MAX;

        uint32_t index = Texture_handle::invalidIndex;

        bool IsValid() const { return this->index == Texture_handle::invalidIndex; }
    };

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

    class PassBuilder {
        friend class FrameGraph;

        FrameGraph &frameGraph;
        uint32_t passIndex;

        PassBuilder(FrameGraph &frameGraph, uint32_t passIndex) 
            : frameGraph(frameGraph), passIndex(passIndex) {}

    public:
        Texture_handle Read(const Texture_handle &handle);
        Texture_handle Write(const Texture_handle &handle);

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

        ID3D11ShaderResourceView *GetShaderResourceView(Texture_handle handle) const;
        ID3D11RenderTargetView *GetRenderTargetView(Texture_handle handle) const;
        ID3D11DepthStencilView *GetDepthStencilView(Texture_handle handle) const;
        ID3D11UnorderedAccessView *GetUnorderedAccessView(Texture_handle handle) const;
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

        uint32_t firstWrite = UINT32_MAX;
        uint32_t lastRead   = UINT32_MAX;
        int readRefCount    = 0;
    };

    struct Render_pass_base {
        std::string name;

        std::vector<uint32_t> reads;
        std::vector<uint32_t> writes;

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
    std::vector<uint32_t> sortedRenderPassIndices;

    int backbufferWidth;
    int backbufferHeight;
    bool isCompiled;

    void CullUnusedPasses();
    void TopologicalSort();

    void ComputeResourceLifetimes();
    void CreateTemporaryResources();
    void ReleaseTemporaryResources();

public:
    FrameGraph();
    ~FrameGraph();

    FrameGraph(const FrameGraph &other) = delete;
    FrameGraph &operator=(const FrameGraph &other) = delete;

    void SetDevice(ID3D11Device *device);

    Texture_handle CreateTexture(const std::string &name, Texture_desc desc);

    Texture_handle ImportTexture(
        const std::string &name,
        ID3D11Texture2D *texture,
        ID3D11RenderTargetView *renderTargetView,
        ID3D11ShaderResourceView *shaderResourceView = nullptr,
        ID3D11DepthStencilView *depthStencilView = nullptr,
        ID3D11UnorderedAccessView *unorderedAccessView = nullptr
    );

    template <typename PassData>
    PassData &AddRenderPass(
        const std::string &name,
        std::function<void(PassData &, PassBuilder &)> setupFunc,
        std::function<void(const PassData &, ExecutionContext &)> executeFunc
    ) {
        uint32_t index = this->renderPasses.size();

        Render_pass<PassData> *renderPass = new Render_pass<PassData>();
        renderPass->name = name;

        this->renderPasses.emplace_back(renderPass);

        PassBuilder builder(*this, index);
        setupFunc(renderPass->data, builder);

        renderPass->executeFunc = std::move(executeFunc);

        return renderPass->data;
    }

    void Compile(int backbufferWidth, int backbufferHeight);
    void OnResize(int width, int height);

    void Execute(ID3D11DeviceContext *deviceContext, const RenderQueue &renderQueue);

    void UpdateImportedTexture(
        Texture_handle handle,
        ID3D11Texture2D *texture,
        ID3D11RenderTargetView *renderTargetView,
        ID3D11ShaderResourceView *shaderResourceView = nullptr,
        ID3D11DepthStencilView *depthStencilView = nullptr,
        ID3D11UnorderedAccessView *unorderedAccessView = nullptr
    );

    void Clear();

    void LogGraph() const;
};

#endif