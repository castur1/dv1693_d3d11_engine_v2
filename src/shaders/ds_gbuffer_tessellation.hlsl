// ds_gbuffer.hlsl
//
// Domain shader for the tessellated G-buffer pass.
//
// Runs once per tessellated vertex. Responsibilities:
//   1. Barycentric interpolation of world-space attributes
//   2. Displacement mapping: sample the heightmap along the interpolated
//      normal, scale by displacementScale, and offset the world position
//   3. Final clip-space projection
//
// Output struct semantics match Pixel_shader_input in ps_gbuffer.hlsl
// exactly, so that PS is reused without modification.

cbuffer Per_frame : register(b0)
{
    float4x4 viewMatrix;
    float4x4 invViewMatrix;
    float4x4 projectionMatrix;
    float4x4 invProjectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
};

cbuffer Tessellation_data : register(b3)
{
    float tessMinFactor;
    float tessMaxFactor;
    float tessMinDistance;
    float tessMaxDistance;
    float displacementScale;
    float3 pad1;
};

// Displacement map at t2; t0 and t1 are diffuse and normal (bound in PS).
// Using SampleLevel since DS has no implicit LOD derivatives.
Texture2D displacementMap : register(t2);
SamplerState linearSampler : register(s0);

struct Control_point
{
    float3 worldPos : POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

struct HS_Constant_data
{
    float edgeTess[3] : SV_TessFactor;
    float insideTess : SV_InsideTessFactor;
};

// Matches ps_gbuffer.hlsl Pixel_shader_input exactly
struct DS_Output
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

[domain("tri")]
DS_Output main(
    HS_Constant_data patchConst,
    float3 bary : SV_DomainLocation,
    const OutputPatch<Control_point, 3> patch)
{
    DS_Output output;

    // ---- Barycentric interpolation ----------------------------------------
    float3 worldPos = bary.x * patch[0].worldPos + bary.y * patch[1].worldPos + bary.z * patch[2].worldPos;
    float3 normal = bary.x * patch[0].normal + bary.y * patch[1].normal + bary.z * patch[2].normal;
    float3 tangent = bary.x * patch[0].tangent + bary.y * patch[1].tangent + bary.z * patch[2].tangent;
    float3 bitangent = bary.x * patch[0].bitangent + bary.y * patch[1].bitangent + bary.z * patch[2].bitangent;
    float2 uv = bary.x * patch[0].uv + bary.y * patch[1].uv + bary.z * patch[2].uv;

    // Re-normalise after interpolation to keep unit length across the patch
    normal = normalize(normal);
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);

    // ---- Displacement mapping --------------------------------------------
    // Heightmap is expected in [0, 1] range.
    // displacementScale controls the maximum offset in world units.
    // Sampling mip 0 explicitly — no implicit gradients available in DS.
    float height = displacementMap.SampleLevel(linearSampler, uv, 0).r;
    worldPos += normal * (height * displacementScale);

    // ---- Projection -------------------------------------------------------
    output.position = mul(float4(worldPos, 1.0f), viewProjectionMatrix);

    output.normal = normal;
    output.tangent = tangent;
    output.bitangent = bitangent;
    output.uv = uv;

    return output;
}
