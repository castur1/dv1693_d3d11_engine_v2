SamplerState samplerLinearWrap : register(s0);

cbuffer Particle_visual : register(b0) {
    float4 startColour;
    float4 endColour;
    float startSize;
    float endSize;
    float nearPlane;
    float farPlane;
};

Texture2D<float4> particleTexture : register(t0);
Texture2D<float> depthTexture : register(t1);

struct Pixel_shader_input {
    float4 positionClip : SV_POSITION;
    float2 uv : TEXCOORD;
    float age : AGE;
};

float LinearizeDepth(float d) {
    return (nearPlane * farPlane) / (farPlane - d * (farPlane - nearPlane));
}

float4 main(Pixel_shader_input input) : SV_TARGET {
    float4 emitterColour = lerp(startColour, endColour, input.age);
    float4 texColour = particleTexture.Sample(samplerLinearWrap, input.uv);
    float4 colour = texColour * emitterColour;

    const int2 pixel = int2(input.positionClip.xy);
    const float sceneDepthNDC = depthTexture.Load(int3(pixel, 0));
    const float particleDepthNDC = input.positionClip.z;

    const float sceneLinear = LinearizeDepth(sceneDepthNDC);
    const float particleLinear = LinearizeDepth(particleDepthNDC);

    const float softRange = 0.5f;
    const float softFactor = saturate((sceneLinear - particleLinear) / softRange);
    colour.a *= softFactor;

    return colour;
}
