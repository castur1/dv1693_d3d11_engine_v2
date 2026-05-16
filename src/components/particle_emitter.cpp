#include "particle_emitter.hpp"
#include "rendering/renderer.hpp"
#include "components/transform.hpp"
#include "scene/entity.hpp"

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

bool ParticleEmitter::CreateResources() {
    if (!this->device) {
        LogWarn("Device was nullptr\n");
        return false;
    }

    if (!this->deviceContext) {
        LogWarn("Device context was nullptr\n");
        return false;
    }

    if (this->maxParticleCount == 0) {
        LogWarn("Max particle count was zero\n");
        return false;
    }

    this->ReleaseResources();

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(Particle) * this->maxParticleCount;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(Particle);

    std::vector<uint8_t> data(desc.ByteWidth, 0);
    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = data.data();

    HRESULT result = this->device->CreateBuffer(&desc, &initialData, &this->particleBuffer);
    if (FAILED(result)) {
        LogWarn("Failed to create particle buffer\n");
        return false;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = this->maxParticleCount;

    result = this->device->CreateUnorderedAccessView(this->particleBuffer, &uavDesc, &this->particleBufferUAV);
    if (FAILED(result)) {
        LogWarn("Failed to create particle buffer UAV\n");
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = this->maxParticleCount;

    result = this->device->CreateShaderResourceView(this->particleBuffer, &srvDesc, &this->particleBufferSRV);
    if (FAILED(result)) {
        LogWarn("Failed to create particle buffer SRV\n");
        return false;
    }

    return true;
}

void ParticleEmitter::ReleaseResources() {
    SafeRelease(this->particleBufferSRV);
    SafeRelease(this->particleBufferUAV);
    SafeRelease(this->particleBuffer);
}

float ParticleEmitter::RandomFloat(float minValue, float maxValue) {
    std::uniform_real_distribution<float> dist(minValue, maxValue);
    return dist(this->rng);
}

ParticleEmitter::~ParticleEmitter() {
    this->ReleaseResources();
}

void ParticleEmitter::OnStart(const Engine_context &context) {
    this->device = context.renderer->GetDevice();
    this->deviceContext = context.renderer->GetDeviceContext();

    if (!this->CreateResources())
        return;

    this->textureHandle = context.assetManager->GetHandle<Texture2D>(this->textureHandle.GetID());
}

void ParticleEmitter::Update(const Frame_context &context) {
    if (!this->isActive)
        return;

    if (!this->particleBuffer)
        return;

    this->lastDeltaTime = context.deltaTime;

    if (this->maxParticleCount < this->spawnRate * this->maxLifetime)
        LogWarn("Particle buffer is too small; live particles might get overwritten");

    Transform *transform = this->GetOwner()->GetComponent<Transform>();

    this->spawnAccumulator += this->spawnRate * context.deltaTime;
    while (this->spawnAccumulator >= 1.0f) {
        this->spawnAccumulator -= 1.0f;

        Particle particle{};
        particle.position = {0.0f, 0.0f, 0.0f}; // TODO: offset
        particle.maxLifetime = this->RandomFloat(this->minLifetime, this->maxLifetime);
        particle.velocity = {
            this->RandomFloat(this->minVelocity.x, this->maxVelocity.x),
            this->RandomFloat(this->minVelocity.y, this->maxVelocity.y),
            this->RandomFloat(this->minVelocity.z, this->maxVelocity.z)
        };
        particle.lifetime = particle.maxLifetime;

        if (transform) {
            particle.position = transform->TransformPoint(particle.position);
            particle.velocity = transform->TransformDirection(particle.velocity);
        }

        const UINT byteOffset = this->nextSpawnIndex * sizeof(Particle);

        D3D11_BOX box{};
        box.left = byteOffset;
        box.right = byteOffset + sizeof(Particle);
        box.top = box.front = 0;
        box.bottom = box.back = 1;

        this->deviceContext->UpdateSubresource(this->particleBuffer, 0, &box, &particle, 0, 0);

        this->nextSpawnIndex = (this->nextSpawnIndex + 1) % this->maxParticleCount;
    }
}

void ParticleEmitter::Render(const Render_view &view, RenderQueue &queue) {
    if (!this->isActive)
        return;

    if (!this->particleBuffer)
        return;

    // TODO: Should particles show up in reflection/shadows/etc.?
    if (view.type != View_type::primary)
        return;

    Particle_emitter_command command{};
    command.buffer              = this->particleBuffer;
    command.unorderedAccessView = this->particleBufferUAV;
    command.shaderResourceView  = this->particleBufferSRV;
    command.maxParticleCount    = this->maxParticleCount;
    command.acceleration        = this->acceleration;
    command.deltaTime           = this->lastDeltaTime;
    command.startColour         = this->startColour;
    command.endColour           = this->endColour;
    command.startSize           = this->startSize;
    command.endSize             = this->endSize;
    command.textureHandle       = this->textureHandle;

    queue.Submit(command);
}

void ParticleEmitter::OnDestroy(const Engine_context &context) {
    this->ReleaseResources();
}