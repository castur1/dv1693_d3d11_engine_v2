struct Particle {
    float3 position;
    float maxLifetime;
    float3 velocity;
    float lifetime;
};

StructuredBuffer<Particle> particles : register(t0);

struct Vertex_shader_output {
    float3 positionWorld : POSITION;
    float age : AGE;
    uint isAlive : ALIVE;
};

Vertex_shader_output main(uint vertexID : SV_VertexID) {
    Vertex_shader_output output;

    Particle particle = particles[vertexID];

    output.positionWorld = particle.position;
    output.isAlive = particle.lifetime > 0.0f;
    output.age = saturate(1.0f - particle.lifetime / particle.maxLifetime);

    return output;
}
