#include "frame_graph.hpp"
#include "core/logging.hpp"

#include <queue>

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

FrameGraph::Render_pass_base *FrameGraph::RenderPassBuilder::GetRenderPass() {
    return this->frameGraph.renderPasses[this->passHandle].get();
}

FrameGraph::TextureHandle FrameGraph::RenderPassBuilder::Read(TextureHandle textureHandle) {
    if (textureHandle == INVALID_HANDLE)
        return INVALID_HANDLE;

    this->GetRenderPass()->reads.insert(textureHandle);

    return textureHandle;
}

FrameGraph::TextureHandle FrameGraph::RenderPassBuilder::Write(TextureHandle textureHandle) {
    if (textureHandle == INVALID_HANDLE)
        return INVALID_HANDLE;

    this->GetRenderPass()->writes.insert(textureHandle);

    return textureHandle;
}

void FrameGraph::RenderPassBuilder::WritesBackbuffer() {
    this->frameGraph.renderPasses[this->passHandle]->writesBackbuffer = true;
}

ID3D11ShaderResourceView *FrameGraph::ExecutionContext::GetShaderResourceView(TextureHandle handle) const {
    if (handle == INVALID_HANDLE || handle >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle].shaderResourceView;
}

ID3D11RenderTargetView *FrameGraph::ExecutionContext::GetRenderTargetView(TextureHandle handle) const {
    if (handle == INVALID_HANDLE || handle >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle].renderTargetView;
}

ID3D11DepthStencilView *FrameGraph::ExecutionContext::GetDepthStencilView(TextureHandle handle) const {
    if (handle == INVALID_HANDLE || handle >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle].depthStencilView;
}

ID3D11UnorderedAccessView *FrameGraph::ExecutionContext::GetUnorderedAccessView(TextureHandle handle) const {
    if (handle == INVALID_HANDLE || handle >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle].unorderedAccessView;
}

FrameGraph::FrameGraph() : device(nullptr), backbufferWidth(0), backbufferHeight(0), isCompiled(false) {}

FrameGraph::~FrameGraph() {
    this->Clear();
}

void FrameGraph::SetDevice(ID3D11Device *device) {
    this->device = device;
}

void FrameGraph::CullUnusedPasses() {
    for (auto &pass : this->renderPasses) {
        pass->isCulled = false;
        pass->refCount = 0;
    }

    for (auto &resource : this->textureResources)
        resource.readRefCount = 0;

    for (const auto &pass : this->renderPasses)
        for (TextureHandle textureHandle : pass->reads)
            if (textureHandle < this->textureResources.size())
                ++this->textureResources[textureHandle].readRefCount;

    for (auto &pass : this->renderPasses) {
        if (pass->writesBackbuffer)
            ++pass->refCount;

        for (TextureHandle textureHandle : pass->writes)
            if (textureHandle < this->textureResources.size() && this->textureResources[textureHandle].readRefCount > 0)
                ++pass->refCount;
    }

    std::queue<PassHandle> cullQueue;
    for (PassHandle passHandle = 0; passHandle < this->renderPasses.size(); ++passHandle)
        if (this->renderPasses[passHandle]->refCount == 0)
            cullQueue.push(passHandle);

    while (!cullQueue.empty()) {
        PassHandle passHandle = cullQueue.front();
        cullQueue.pop();

        Render_pass_base *pass = this->renderPasses[passHandle].get();
        pass->isCulled = true;

        for (TextureHandle textureHandle : pass->reads) {
            if (textureHandle >= this->textureResources.size())
                continue;

            if (--this->textureResources[textureHandle].readRefCount > 0)
                continue;

            for (PassHandle otherPassHandle = 0; otherPassHandle < this->renderPasses.size(); ++otherPassHandle) {
                if (this->renderPasses[otherPassHandle]->isCulled)
                    continue;

                if (this->renderPasses[otherPassHandle]->writes.count(textureHandle))
                    if (--this->renderPasses[otherPassHandle]->refCount == 0)
                        cullQueue.push(otherPassHandle);
            }
        }
    }
}

// https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm
bool FrameGraph::TopologicalSort() {
    this->sortedPassHandles.clear();

    const uint32_t passCount = this->renderPasses.size();

    // Construct the graph

    std::vector<std::unordered_set<PassHandle>> dependencies(passCount);
    std::vector<std::unordered_set<PassHandle>> successors(passCount);

    for (PassHandle passHandle = 0; passHandle < passCount; ++passHandle) {
        if (this->renderPasses[passHandle]->isCulled)
            continue;

        for (TextureHandle textureHandle : this->renderPasses[passHandle]->reads) {
            for (PassHandle otherPassHandle = 0; otherPassHandle < passCount; ++otherPassHandle) {
                if (otherPassHandle == passHandle || this->renderPasses[otherPassHandle]->isCulled)
                    continue;

                if (this->renderPasses[otherPassHandle]->writes.count(textureHandle))
                    if (dependencies[passHandle].insert(otherPassHandle).second)
                        successors[otherPassHandle].insert(passHandle);
            }
        }
    }

    // Sort the graph

    std::vector<int> inDegree(passCount, 0);
    for (PassHandle passHandle = 0; passHandle < passCount; ++passHandle)
        if (!this->renderPasses[passHandle]->isCulled)
            inDegree[passHandle] = dependencies[passHandle].size();

    std::queue<PassHandle> readyQueue;
    for (PassHandle passHandle = 0; passHandle < passCount; ++passHandle)
        if (!this->renderPasses[passHandle]->isCulled && inDegree[passHandle] == 0)
            readyQueue.push(passHandle);

    while (!readyQueue.empty()) {
        PassHandle passHandle = readyQueue.front();
        readyQueue.pop();

        this->sortedPassHandles.push_back(passHandle);

        for (PassHandle successor : successors[passHandle])
            if (--inDegree[successor] == 0)
                readyQueue.push(successor);
    }

    int liveCount = 0; 
    for (const auto &pass : this->renderPasses)
        if (!pass->isCulled)
            ++liveCount;

    if (this->sortedPassHandles.size() != liveCount) {
        LogError("Cycle detected in render pass dependency graph");
        return false;
    }

    return true;
}

void FrameGraph::ComputeResourceLifetimes() {
    for (auto &resource : this->textureResources) {
        resource.firstWrite = INVALID_HANDLE;
        resource.lastRead   = INVALID_HANDLE;
    }

    for (int i = 0; i < this->sortedPassHandles.size(); ++i) {
        PassHandle passHandle = this->sortedPassHandles[i];
        const Render_pass_base *pass = this->renderPasses[passHandle].get();

        for (TextureHandle textureHandle : pass->writes)
            if (textureHandle < this->textureResources.size() && this->textureResources[textureHandle].firstWrite == INVALID_HANDLE)
                this->textureResources[textureHandle].firstWrite = i;

        for (TextureHandle textureHandle : pass->reads)
            if (textureHandle < this->textureResources.size())
                this->textureResources[textureHandle].lastRead = i;
    }
}

static bool IsDepthStencilFormat(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;

        default:
            return false;
    }
}

static DXGI_FORMAT GetTypelessDepthStencilFormat(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32G8X24_TYPELESS;

        default:
            return format;
    }
}

static DXGI_FORMAT GetDepthStencilSRVFormat(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

        default:
            return format;
    }
}

void FrameGraph::CreateTemporaryResources() {
    for (auto &resource : this->textureResources) {
        if (resource.wasImported)
            continue;

        if (resource.firstWrite == INVALID_HANDLE)
            continue;

        UINT width, height;
        if (resource.desc.sizeMode == Texture_desc::Size_mode::absolute) {
            width  = resource.desc.width;
            height = resource.desc.height;
        }
        else {
            width  = this->backbufferWidth  * resource.desc.widthScale;
            height = this->backbufferHeight * resource.desc.heightScale;
        }

        width  = width  < 1u ? 1u : width;
        height = height < 1u ? 1u : height;

        const bool bindsDepthStencil    = (resource.desc.bindFlags & D3D11_BIND_DEPTH_STENCIL)    != 0;
        const bool bindsShaderResource  = (resource.desc.bindFlags & D3D11_BIND_SHADER_RESOURCE)  != 0;
        const bool bindsRenderTarget    = (resource.desc.bindFlags & D3D11_BIND_RENDER_TARGET)    != 0;
        const bool bindsUnorderedAccess = (resource.desc.bindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0;

        DXGI_FORMAT format = resource.desc.format;
        if (bindsDepthStencil && bindsShaderResource)
            format = GetTypelessDepthStencilFormat(format);

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width              = width;
        desc.Height             = height;
        desc.MipLevels          = resource.desc.mipLevels;
        desc.ArraySize          = 1;
        desc.Format             = format;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage              = D3D11_USAGE_DEFAULT;
        desc.BindFlags          = resource.desc.bindFlags;
        desc.CPUAccessFlags     = 0;
        desc.MiscFlags          = 0;

        HRESULT result = this->device->CreateTexture2D(&desc, nullptr, &resource.texture);
        if (FAILED(result)) {
            LogWarn("Failed to create texture '%s'\n", resource.name.c_str());
            continue;
        }

        if (bindsRenderTarget && !bindsDepthStencil) {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = resource.desc.format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;

            result = this->device->CreateRenderTargetView(resource.texture, &rtvDesc, &resource.renderTargetView);
            if (FAILED(result))
                LogWarn("Failed to create Render Target View for '%s'\n", resource.name.c_str());
        }

        if (bindsDepthStencil) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format = resource.desc.format;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;

            result = this->device->CreateDepthStencilView(resource.texture, &dsvDesc, &resource.depthStencilView);
            if (FAILED(result))
                LogWarn("Failed to create Depth Stencil View for '%s'\n", resource.name.c_str());
        }

        if (bindsShaderResource) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = bindsDepthStencil ? GetDepthStencilSRVFormat(resource.desc.format) : resource.desc.format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = resource.desc.mipLevels;

            result = this->device->CreateShaderResourceView(resource.texture, &srvDesc, &resource.shaderResourceView);
            if (FAILED(result))
                LogWarn("Failed to create Shader Resource View for '%s'\n", resource.name.c_str());
        }

        if (bindsUnorderedAccess) {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = resource.desc.format;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;

            result = this->device->CreateUnorderedAccessView(resource.texture, &uavDesc, &resource.unorderedAccessView);
            if (FAILED(result))
                LogWarn("Failed to create Unordered Access View for '%s'\n", resource.name.c_str());
        }
    }
}

void FrameGraph::ReleaseTemporaryResources() {
    for (auto &resource : this->textureResources) {
        if (resource.wasImported)
            continue;

        SafeRelease(resource.unorderedAccessView);
        SafeRelease(resource.shaderResourceView);
        SafeRelease(resource.renderTargetView);
        SafeRelease(resource.depthStencilView);
        SafeRelease(resource.texture);
    }
}

FrameGraph::TextureHandle FrameGraph::CreateTexture(const std::string &name, FrameGraph::Texture_desc desc) {
    TextureHandle handle = this->textureResources.size();

    Texture_resource resource{};
    resource.name = name;
    resource.desc = desc;
    resource.wasImported = false;

    this->textureResources.push_back(resource);

    return handle;
}

FrameGraph::TextureHandle FrameGraph::ImportTexture(
    const std::string &name,
    ID3D11Texture2D *texture,
    ID3D11RenderTargetView *renderTargetView,
    ID3D11ShaderResourceView *shaderResourceView,
    ID3D11DepthStencilView *depthStencilView,
    ID3D11UnorderedAccessView *unorderedAccessView
) {
    TextureHandle handle = this->textureResources.size();

    Texture_resource resource{};
    resource.name = name;
    resource.wasImported = true;
    resource.texture = texture;
    resource.renderTargetView = renderTargetView;
    resource.shaderResourceView = shaderResourceView;
    resource.depthStencilView = depthStencilView;
    resource.unorderedAccessView = unorderedAccessView;

    this->textureResources.push_back(resource);

    return handle;
}

void FrameGraph::UpdateImportedTexture(
    TextureHandle handle,
    ID3D11Texture2D *texture,
    ID3D11RenderTargetView *renderTargetView,
    ID3D11ShaderResourceView *shaderResourceView,
    ID3D11DepthStencilView *depthStencilView,
    ID3D11UnorderedAccessView *unorderedAccessView
) {
    if (handle == INVALID_HANDLE || handle >= this->textureResources.size()) {
        LogWarn("Invalid handle\n");
        return;
    }

    Texture_resource &resource = this->textureResources[handle];
    if (!resource.wasImported) {
        LogWarn("Tried to update non-imported texture resource '%s'\n", resource.name.c_str());
        return;
    }

    resource.texture = texture;
    resource.renderTargetView = renderTargetView;
    resource.shaderResourceView = shaderResourceView;
    resource.depthStencilView = depthStencilView;
    resource.unorderedAccessView = unorderedAccessView;
}

void FrameGraph::Compile(int backbufferWidth, int backbufferHeight) {
    LogInfo("Compiling frame graph... (%dx%d)\n", backbufferWidth, backbufferHeight);
    LogIndent();

    this->isCompiled = false;

    this->backbufferWidth  = backbufferWidth;
    this->backbufferHeight = backbufferHeight;

    this->ReleaseTemporaryResources();
    this->CullUnusedPasses();
    this->TopologicalSort();
    this->ComputeResourceLifetimes();
    this->CreateTemporaryResources();

    this->isCompiled = true;

    LogUnindent();
    this->LogGraph();
}

void FrameGraph::OnResize(int width, int height) {
    LogInfo("Resizing frame graph\n");
    this->Compile(width, height);
}

void FrameGraph::Execute(ID3D11DeviceContext *deviceContext, const RenderQueue &renderQueue) {
    if (!this->isCompiled) {
        LogWarn("Tried to execute uncompiled frame graph!\n");
        return;
    }

    ExecutionContext context(deviceContext, renderQueue, *this);

    for (PassHandle passHandle : this->sortedPassHandles)
        this->renderPasses[passHandle]->Execute(context);
}

void FrameGraph::Clear() {
    this->ReleaseTemporaryResources();

    this->renderPasses.clear();
    this->textureResources.clear();
    this->sortedPassHandles.clear();

    this->isCompiled = false;
}

void FrameGraph::LogGraph() const {
    int culledCount = 0;
    for (const auto &pass : this->renderPasses)
        if (pass->isCulled)
            ++culledCount;

    LogInfo(
        "Frame graph: %zu passes (%d culled), %zu resources\n", 
        this->renderPasses.size(), 
        culledCount, 
        this->textureResources.size()
    );
}
