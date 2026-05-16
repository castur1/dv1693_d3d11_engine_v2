struct Particle {
    float3 position;
    float maxLifetime;
    float3 velocity;
    float lifetime;
};

StructuredBuffer<Particle> particles : register(t0);


struct VS_Output
{
    float3 worldPosition : WORLD_POSITION;
    float age : AGE;
    uint isAlive : IS_ALIVE;
};

VS_Output main(uint vertexID : SV_VertexID)
{
    VS_Output output;

    Particle p = particles[vertexID];

    output.worldPosition = p.position;
    output.isAlive = p.lifetime > 0.0f;
    output.age = saturate(1.0f - p.lifetime / p.maxLifetime);

    return output;
}
