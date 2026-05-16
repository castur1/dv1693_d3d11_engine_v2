Texture2D<float4> hdrBuffer : register(t0);

// Debug
Texture2D<float4> gbufferAlbedo : register(t1);
Texture2D<float4> gbufferNormal : register(t2);
Texture2D<float4> gbufferSpecular : register(t3);
Texture2D<float4> depthBuffer : register(t4);

SamplerState linearSampler : register(s0);

cbuffer ResolveData : register(b3) {
    int debugMode;
    float nearPlane;
    float farPlane;
    float pad0;
};

float LineariseDepth(float d) {
    return (nearPlane * farPlane) / (farPlane - d * (farPlane - nearPlane)) / farPlane;
}

float Luminance(float3 colour) {
    return dot(colour, float3(0.2126f, 0.7152f, 0.0722f));
}

float4 SampleQuadrant(float2 uv) {
    int2 quadrant = floor(uv * 2);
    int index = quadrant.x + 2 * quadrant.y;
    
    float2 localUV = frac(uv * 2);
    
    if (index == 0)
        return float4(gbufferAlbedo.Sample(linearSampler, localUV).rgb, 1.0f);

    if (index == 1)
        return float4(gbufferNormal.Sample(linearSampler, localUV).rgb, 1.0f);
    
    if (index == 2)
        return float4(sqrt(LineariseDepth(depthBuffer.Sample(linearSampler, localUV).r)).rrr, 1.0f);
    
    float3 hdr = hdrBuffer.Sample(linearSampler, localUV).rgb;

    // https://64.github.io/tonemapping/
    // Reinhard-Jodie
    float l = Luminance(hdr);
    float3 tv = hdr / (1.0f + hdr);
    float3 ldr = lerp(hdr / (1.0f + l), tv, tv);

    // Gamma correction
    ldr = pow(saturate(ldr), 1.0f / 2.2f);

    return float4(ldr, 1.0f);
}

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {
    if (debugMode == 1)
        return float4(gbufferAlbedo.Sample(linearSampler, uv).rgb, 1.0f);
    if (debugMode == 2)
        return float4(gbufferNormal.Sample(linearSampler, uv).rgb, 1.0f);
    if (debugMode == 3)
        return float4(gbufferSpecular.Sample(linearSampler, uv).rgb, 1.0f);
    if (debugMode == 4)
        return float4(sqrt(LineariseDepth(depthBuffer.Sample(linearSampler, uv).r)).rrr, 1.0f);
    if (debugMode == 5)
        return SampleQuadrant(uv);
    
    // ^^^ Debug stuff ^^^
    
    float3 hdr = hdrBuffer.Sample(linearSampler, uv).rgb;
    
    hdr = max(hdr, 0.0f);

    // https://64.github.io/tonemapping/
    // Reinhard-Jodie
    float l = Luminance(hdr);
    float3 tv = hdr / (1.0f + hdr);
    float3 ldr = lerp(hdr / (1.0f + l), tv, tv);

    // Gamma correction
    ldr = pow(saturate(ldr), 1.0f / 2.2f);

    return float4(ldr, 1.0f);
}
