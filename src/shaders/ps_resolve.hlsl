Texture2D<float4> hdrBuffer : register(t0);

SamplerState linearSampler : register(s0);

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {
    float3 hdr = hdrBuffer.Sample(linearSampler, uv).rgb;
    return float4(hdr, 1.0f);

    // float3 ldr = hdr / (hdr + 1.0f);

    // ldr = pow(saturate(ldr), 1.0f / 2.2f);

    // return float4(ldr, 1.0f);
}
