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

float3 SampleQuadrant(float2 uv) {
    int2 quadrant = floor(uv * 2);
    int index = quadrant.x + 2 * quadrant.y;
    
    float2 localUV = frac(uv * 2);
    
    if (index == 0)
        return gbufferAlbedo.Sample(linearSampler, localUV).rgb;

    if (index == 1)
        return gbufferNormal.Sample(linearSampler, localUV).rgb;
    
    if (index == 2)
        return sqrt(LineariseDepth(depthBuffer.Sample(linearSampler, localUV).r)).rrr;
    
    float3 hdr = hdrBuffer.Sample(linearSampler, localUV).rgb;

    // https://64.github.io/tonemapping/
    // Reinhard-Jodie
    //float l = Luminance(hdr);
    //float3 tv = hdr / (1.0f + hdr);
    //return lerp(hdr / (1.0f + l), tv, tv);
    return hdr;
}

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {
    float3 ldr = float3(0.0f, 0.0f, 0.0f);
    
    if (debugMode == 1)
        ldr = gbufferAlbedo.Sample(linearSampler, uv).rgb;
    else if (debugMode == 2)
        ldr = gbufferNormal.Sample(linearSampler, uv).rgb;
    else if (debugMode == 3)
        ldr = gbufferSpecular.Sample(linearSampler, uv).rgb;
    else if (debugMode == 4)
        ldr = LineariseDepth(depthBuffer.Sample(linearSampler, uv).r).rrr;
    else if (debugMode == 5)
        ldr = SampleQuadrant(uv);
    else {
        float3 hdr = max(hdrBuffer.Sample(linearSampler, uv).rgb, 0.0f);
    
        // https://64.github.io/tonemapping/
        // Reinhard-Jodie
        //float l = Luminance(hdr);
        //float3 tv = hdr / (1.0f + hdr);
        //ldr = lerp(hdr / (1.0f + l), tv, tv);
        ldr = hdr;
    }

    // Gamma correction
    // Could be done automatically by the hardware if we create the backbuffer as sRGB,
    // but that isn't compatible with ImGui unfortunately
    ldr = pow(saturate(ldr), 1.0f / 2.2f);

    return float4(ldr, 1.0f);
}
