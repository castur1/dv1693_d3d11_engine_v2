struct Particle {
    float3 position;
    float maxLifetime;
    float3 velocity;
    float lifetime;
};

cbuffer Particle_compute : register(b0) {
    float3 acceleration;
    float deltaTime;
    uint maxParticles;
    float3 pad0;
};

RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    const uint index = id.x;
    if (index >= maxParticles)
        return;

    Particle particle = particles[index];

    particle.lifetime -= deltaTime;
    if (particle.lifetime <= 0.0f) {
        particle.lifetime = 0.0f;
        
        particles[index] = particle;
        
        return;
    }
    
    particle.velocity += acceleration * deltaTime;
    particle.position += particle.velocity * deltaTime;

    particles[index] = particle;
}
