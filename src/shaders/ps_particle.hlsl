cbuffer Particle_visual_data : register(b0) {
    float4 startColor;
    float4 endColor;
    float startSize;
    float endSize;
    float nearPlane;
    float farPlane;
};

Texture2D<float4> particleTexture : register(t0);
Texture2D<float> depthTexture : register(t1);
SamplerState linearSampler : register(s0);

struct PS_Input {
    float4 clipPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float age : AGE;
};

float LinearizeDepth(float d) {
    return (nearPlane * farPlane) / (farPlane - d * (farPlane - nearPlane));
}

float4 main(PS_Input input) : SV_TARGET {   
    float4 emitterColor = lerp(startColor, endColor, input.age);
    float4 texColor = particleTexture.Sample(linearSampler, input.uv);
    float4 color = texColor * emitterColor;

    const int2 pixelCoord = int2(input.clipPosition.xy);
    const float sceneDepthNDC = depthTexture.Load(int3(pixelCoord, 0));
    const float particleDepthNDC = input.clipPosition.z;

    const float sceneLinear = LinearizeDepth(sceneDepthNDC);
    const float particleLinear = LinearizeDepth(particleDepthNDC);

    const float softRange = 1.0f;

    const float softFade = saturate((sceneLinear - particleLinear) / softRange);

    color.a *= softFade;

    return color;
}
