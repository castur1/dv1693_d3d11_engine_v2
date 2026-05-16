#ifndef PARTICLE_EMITTER_HPP
#define PARTICLE_EMITTER_HPP

#include "scene/component.hpp"
#include "resources/asset_manager.hpp"
#include "resources/model.hpp"

#include "d3d11.h"
#include "DirectXMath.h"
#include "random"
#include "vector"

class ParticleEmitter : public Component {
    // Structured buffer element
    struct Particle {
        XMFLOAT3 position;
        float maxLifetime;
        XMFLOAT3 velocity;
        float lifetime;
    };
    static_assert(sizeof(Particle) % 16 == 0);

    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *deviceContext = nullptr;

    ID3D11Buffer *particleBuffer = nullptr;
    ID3D11UnorderedAccessView *particleBufferUAV = nullptr;
    ID3D11ShaderResourceView *particleBufferSRV = nullptr;

    float spawnAccumulator = 0.0f;
    UINT nextSpawnIndex = 0;
    float lastDeltaTime = 0.0f;

    std::mt19937 rng;

    bool CreateResources();
    void ReleaseResources();
    float RandomFloat(float minValue, float maxValue);

public:
    UINT maxParticleCount = 1000; // TODO: Setting this needs to recreate all resources. Or, this could be const.
    float spawnRate = 50.0f;
    float minLifetime = 1.0f;
    float maxLifetime = 2.0f;

    XMFLOAT3 acceleration = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 minVelocity = {-1.0f, 3.0f, -1.0f};
    XMFLOAT3 maxVelocity = {1.0f, 5.0f, 1.0f};
    // TODO: Start position offset

    XMFLOAT4 startColour = {1.0f, 1.0f, 1.0f, 1.0f};
    XMFLOAT4 endColour = {1.0f, 1.0f, 1.0f, 0.0f};
    float startSize = 0.5f;
    float endSize = 0.0f;

    AssetHandle<Texture2D> textureHandle{};

    ParticleEmitter(Entity *owner, bool isActive) : Component(owner, isActive) {}
    ~ParticleEmitter();

    void OnStart(const Engine_context &context) override;
    void Update(const Frame_context &context) override;
    void Render(const Render_view &view, RenderQueue &queue) override;
    void OnDestroy(const Engine_context &context) override;

    void Reflect(ComponentRegistry::Inspector *inspector) override {
        BIND(maxParticleCount);
        BIND(spawnRate);
        BIND(minLifetime);
        BIND(maxLifetime);

        BIND(acceleration);
        BIND(minVelocity);
        BIND(maxVelocity);

        BIND(startColour);
        BIND(endColour);
        BIND(startSize);
        BIND(endSize);

        AssetID textureID = this->textureHandle.GetID();
        BIND(textureID);
        this->textureHandle.SetID(textureID);
    }
};

REGISTER_COMPONENT(ParticleEmitter);

#endif