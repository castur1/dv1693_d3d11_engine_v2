Texture2D<float4> gbufferAlbedo : register(t0);
Texture2D<float4> gbufferNormal : register(t1);
Texture2D<float4> gbufferSpecular : register(t2);
Texture2D<float4> depthStencil : register(t3);

RWTexture2D<float4> outputTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint width, height;
    outputTexture.GetDimensions(width, height);
    
    if (id.x >= width || id.y >= height)
        return;
    
    int2 pixel = int2(id.xy);
    
    float depth = depthStencil[pixel].r;
    if (depth >= 1.0f)
        return;
    
<<<<<<< HEAD
    float3 albedo = gbufferAlbedo[pixel].rgb;
    float3 normal = gbufferNormal[pixel].rgb;
    float t = 0.4f;
    outputTexture[pixel] = float4(albedo * t + normal * (1 - t), 1.0f);
=======
    outputTexture[pixel] = float4(gbufferNormal[pixel].rgb, 1.0f);
>>>>>>> ac7a8a100ba13c8dbe2c12872267afd00d474438
}