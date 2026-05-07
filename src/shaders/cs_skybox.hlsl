cbuffer Per_frame_data : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
}

TextureCube g_skybox : register(t0);
SamplerState g_sampler : register(s0);
RWTexture2DArray<float4> g_output : register(u0);

// TODO: REWRITE

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint width, height, elements;
    g_output.GetDimensions(width, height, elements);

    if (id.x >= width || id.y >= height)
        return;

    // Reconstruct world-space ray direction for this texel.
    // Texel centre in UV space, then unproject through the face's inv VP matrix.
    float2 uv = (float2(id.xy) + 0.5f) / float2(width, height);
    float4 ndc = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);
    float4 worldPos = mul(ndc, invViewProjectionMatrix);
    worldPos.xyz /= worldPos.w;

    float3 rayDir = normalize(worldPos.xyz - cameraPosition);

    // Slice index 0 — the UAV is created with ArraySize=1 pointing at the target face,
    // so index 0 always refers to the currently bound face.
    g_output[uint3(id.xy, 0)] = float4(g_skybox.SampleLevel(g_sampler, rayDir, 0).rgb, 1.0f);
}