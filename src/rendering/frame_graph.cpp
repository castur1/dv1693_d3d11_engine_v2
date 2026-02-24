#include "frame_graph.hpp"
#include "core/logging.hpp"

#include <queue>
#include <set>

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

// Questions:
// 1. In PassBuilder, why store frameGraph and passIndex rather than the Render_pass_base itself?
// 2. In Render_pass_base, why store reads/writes as a vector rather than unordered_set if we manually check whether we already have the index?
// 3. Should there be support for buffers as a type of resource rather than just textures?
// 4. You say that OnResize() only recreateds transient textures, but it just calls Compile()?
// 5. Why does Render_pass_base reads/writes store uint32_t rather than Texture_handle? Other places do this as well. And UINT32_MAX rather than Texture_handle::invalid
// 6. In TopologicalSort, could we use an unordered_set instead of a normal set? Would that be more optimal?
// 7. In TopologicalSort, why is successors not also a set? Is it because it simply doesn't need to be, since dependencies handles all the duplicate checks for us?
// 8. Why do we check if a render pass is culled rather than simply removing the pass? Is it because that that would alter the other passes' indices?
// 9. What's up with DSV + SRV needing typeless texture format?
// 10. I don't really understand how CreateTemporaryResources (CreateTransientResources) works actually. Why those combinations?
// 11. What about textures/resources that aren't 2D textures? I suppose you'd perhaps create them outside the frame graph and import them if you really needed them.
// Feel free to correct me if I've misunderstood anything. This is decently complex code, so I'm bound to get something wrong haha

FrameGraph::Texture_handle FrameGraph::PassBuilder::Read(const Texture_handle &handle) {
    if (!handle.IsValid())
        return handle;

    Render_pass_base *pass = this->frameGraph.renderPasses[this->passIndex].get();

    if (std::find(pass->reads.begin(), pass->reads.end(), handle.index) == pass->reads.end())
        pass->reads.push_back(handle.index);

    return handle;
}

FrameGraph::Texture_handle FrameGraph::PassBuilder::Write(const Texture_handle &handle) {
    if (!handle.IsValid())
        return handle;

    Render_pass_base *pass = this->frameGraph.renderPasses[this->passIndex].get();

    if (std::find(pass->writes.begin(), pass->writes.end(), handle.index) == pass->writes.end())
        pass->writes.push_back(handle.index);

    return handle;
}

void FrameGraph::PassBuilder::WritesBackbuffer() {
    this->frameGraph.renderPasses[this->passIndex]->writesBackbuffer = true;
}

ID3D11ShaderResourceView *FrameGraph::ExecutionContext::GetShaderResourceView(Texture_handle handle) const {
    if (!handle.IsValid() || handle.index >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle.index].shaderResourceView;
}

ID3D11RenderTargetView *FrameGraph::ExecutionContext::GetRenderTargetView(Texture_handle handle) const {
    if (!handle.IsValid() || handle.index >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle.index].renderTargetView;
}

ID3D11DepthStencilView *FrameGraph::ExecutionContext::GetDepthStencilView(Texture_handle handle) const {
    if (!handle.IsValid() || handle.index >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle.index].depthStencilView;
}

ID3D11UnorderedAccessView *FrameGraph::ExecutionContext::GetUnorderedAccessView(Texture_handle handle) const {
    if (!handle.IsValid() || handle.index >= this->frameGraph.textureResources.size())
        return nullptr;

    return this->frameGraph.textureResources[handle.index].unorderedAccessView;
}

FrameGraph::FrameGraph() 
    : device(nullptr), 
    backbufferWidth(0), 
    backbufferHeight(0), 
    isCompiled(false) {}

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
        for (uint32_t index : pass->reads)
            if (index < this->textureResources.size())
                ++this->textureResources[index].readRefCount;

    for (auto &pass : this->renderPasses) {
        if (pass->writesBackbuffer)
            ++pass->refCount;

        for (uint32_t index : pass->writes)
            if (index < this->textureResources.size() && this->textureResources[index].readRefCount > 0)
                ++pass->refCount;
    }

    std::queue<uint32_t> cullQueue;
    for (uint32_t i = 0; i < this->renderPasses.size(); ++i)
        if (this->renderPasses[i]->refCount == 0)
            cullQueue.push(i);

    while (!cullQueue.empty()) {
        uint32_t passIndex = cullQueue.front();
        cullQueue.pop();

        Render_pass_base *pass = this->renderPasses[passIndex].get();
        pass->isCulled = true;

        for (uint32_t resourceIndex : pass->reads) {
            if (resourceIndex >= this->textureResources.size())
                continue;

            if (--this->textureResources[resourceIndex].readRefCount > 0)
                continue;

            for (uint32_t i = 0; i < this->renderPasses.size(); ++i) {
                if (this->renderPasses[i]->isCulled)
                    continue;

                const auto &writes = this->renderPasses[i]->writes;
                if (std::find(writes.begin(), writes.end(), resourceIndex) != writes.end())
                    if (--this->renderPasses[i]->refCount == 0)
                        cullQueue.push(i);
            }
        }
    }
}

// https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm
bool FrameGraph::TopologicalSort() {
    this->sortedRenderPassIndices.clear();

    const uint32_t passCount = this->renderPasses.size();

    std::vector<std::set<uint32_t>> dependencies(passCount);
    std::vector<std::vector<uint32_t>> successors(passCount);

    for (uint32_t i = 0; i < passCount; ++i) {
        if (this->renderPasses[i]->isCulled)
            continue;

        for (uint32_t resourceIndex : this->renderPasses[i]->reads) {
            for (uint32_t j = 0; j < passCount; ++j) {
                if (j == i || this->renderPasses[j]->isCulled)
                    continue;

                const auto &writes = this->renderPasses[j]->writes;
                if (std::find(writes.begin(), writes.end(), resourceIndex) != writes.end())
                    if (dependencies[i].insert(j).second)
                        successors[j].push_back(i);
            }
        }
    }

    std::vector<int> inDegree(passCount, 0);
    for (uint32_t i = 0; i < passCount; ++i)
        if (!this->renderPasses[i]->isCulled)
            inDegree[i] = dependencies[i].size();

    std::queue<uint32_t> readyQueue;
    for (uint32_t i = 0; i < passCount; ++i)
        if (!this->renderPasses[i]->isCulled && inDegree[i] == 0)
            readyQueue.push(i);

    while (!readyQueue.empty()) {
        uint32_t passIndex = readyQueue.front();
        readyQueue.pop();

        this->sortedRenderPassIndices.push_back(passIndex);

        for (uint32_t successor : successors[passIndex])
            if (--inDegree[successor] == 0)
                readyQueue.push(successor);
    }

    int liveCount = 0; 
    for (const auto &pass : this->renderPasses)
        if (!pass->isCulled)
            ++liveCount;

    if (this->sortedRenderPassIndices.size() != liveCount) {
        LogError("Cycle detected in render pass dependency graph");
        return false;
    }

    return true;
}

void FrameGraph::ComputeResourceLifetimes() {
    for (auto &resource : this->textureResources) {
        resource.firstWrite = UINT32_MAX;
        resource.lastRead = UINT32_MAX;
    }

    for (uint32_t sortedIndex = 0; sortedIndex < this->sortedRenderPassIndices.size(); ++sortedIndex) {
        uint32_t passIndex = this->sortedRenderPassIndices[sortedIndex];
        const Render_pass_base *pass = this->renderPasses[passIndex].get();

        for (uint32_t resourceIndex : pass->writes)
            if (resourceIndex < this->textureResources.size() && this->textureResources[resourceIndex].firstWrite == UINT32_MAX)
                this->textureResources[resourceIndex].firstWrite = sortedIndex;

        for (uint32_t resourceIndex : pass->reads)
            if (resourceIndex < this->textureResources.size())
                this->textureResources[resourceIndex].lastRead = sortedIndex;
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

        if (resource.firstWrite == UINT32_MAX)
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
            rtvDesc.Format             = resource.desc.format;
            rtvDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;

            result = this->device->CreateRenderTargetView(resource.texture, &rtvDesc, &resource.renderTargetView);
            if (FAILED(result))
                LogWarn("Failed to create Render Target View for '%s'\n", resource.name.c_str());
        }

        if (bindsDepthStencil) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format             = resource.desc.format;
            dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
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

FrameGraph::Texture_handle FrameGraph::CreateTexture(const std::string &name, FrameGraph::Texture_desc desc) {
    Texture_handle handle{};
    handle.index = this->textureResources.size();

    Texture_resource resource{};
    resource.name = name;
    resource.desc = desc;
    resource.wasImported = false;

    this->textureResources.push_back(resource);

    return handle;
}

FrameGraph::Texture_handle FrameGraph::ImportTexture(
    const std::string &name,
    ID3D11Texture2D *texture,
    ID3D11RenderTargetView *renderTargetView,
    ID3D11ShaderResourceView *shaderResourceView,
    ID3D11DepthStencilView *depthStencilView,
    ID3D11UnorderedAccessView *unorderedAccessView
) {
    Texture_handle handle{};
    handle.index = this->textureResources.size();

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
    Texture_handle handle,
    ID3D11Texture2D *texture,
    ID3D11RenderTargetView *renderTargetView,
    ID3D11ShaderResourceView *shaderResourceView,
    ID3D11DepthStencilView *depthStencilView,
    ID3D11UnorderedAccessView *unorderedAccessView
) {
    if (!handle.IsValid() || handle.index >= this->textureResources.size()) {
        LogWarn("Invalid handle\n");
        return;
    }

    Texture_resource &resource = this->textureResources[handle.index];
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

    for (uint32_t passIndex : this->sortedRenderPassIndices)
        this->renderPasses[passIndex]->Execute(context);
}

void FrameGraph::Clear() {
    this->ReleaseTemporaryResources();

    this->renderPasses.clear();
    this->textureResources.clear();
    this->sortedRenderPassIndices.clear();

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
