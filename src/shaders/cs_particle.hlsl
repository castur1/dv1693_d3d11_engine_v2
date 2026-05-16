struct Particle {
    float3 position;
    float maxLifetime;
    float3 velocity;
    float lifetime;
};

cbuffer Particle_compute_data : register(b0) {
    float3 acceleration;
    float deltaTime;
    uint maxParticles;
    float3 cbPad;
};

RWStructuredBuffer<Particle> particles : register(u0);


[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    const uint index = DTid.x;
    if (index >= maxParticles)
        return;

    Particle p = particles[index];

    if (p.lifetime <= 0.0f)
        return;

    p.lifetime -= deltaTime;
    if (p.lifetime <= 0.0f) {
        p.lifetime = 0.0f;
        particles[index] = p;
        return;
    }
    p.velocity += acceleration * deltaTime;
    p.position += p.velocity * deltaTime;

    particles[index] = p;
}
