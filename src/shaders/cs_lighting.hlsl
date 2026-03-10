Texture2D<float4> gbufferAlbedo : register(t0);
Texture2D<float4> gbufferNormal : register(t1);
Texture2D<float4> gbufferSpecular : register(t2);
Texture2D<float4> depthBuffer : register(t3);

RWTexture2D<float4> outputTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint width, height;
    outputTexture.GetDimensions(width, height);
    
    if (id.x >= width || id.y >= height)
        return;
    
    int2 pixel = int2(id.xy);
    
    float depth = depthBuffer[pixel].r;
    if (depth >= 1.0f)
        return;
    
    float3 albedo = gbufferAlbedo[pixel].rgb;
    float3 normal = saturate(gbufferNormal[pixel].rgb * 2.0f - 1.0f);
    float t = 0.0f;
    outputTexture[pixel] = float4(albedo * t + normal * (1 - t), 1.0f);
}