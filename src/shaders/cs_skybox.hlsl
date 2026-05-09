SamplerState samplerLinearWrap : register(s0);

cbuffer Per_frame_data : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
}

TextureCube skyboxTexture : register(t0);

RWTexture2DArray<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint width, height, elements;
    output.GetDimensions(width, height, elements);

    if (id.x >= width || id.y >= height)
        return;

    float2 uv = (float2(id.xy) + 0.5f) / float2(width, height);
    float4 ndc = float4(2.0f * uv.x - 1.0f, -2.0f * uv.y + 1.0f, 1.0f, 1.0f);
    float4 positionWorld = mul(ndc, invViewProjectionMatrix);
    positionWorld.xyz /= positionWorld.w;

    float3 direction = normalize(positionWorld.xyz - cameraPosition);

    output[uint3(id.xy, 0)] = float4(skyboxTexture.SampleLevel(samplerLinearWrap, direction, 0).rgb, 1.0f);
}