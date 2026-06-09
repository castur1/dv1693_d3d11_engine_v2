// vs_gbuffer_tess.hlsl
//
// Vertex shader for the tessellated geometry pass.
// Unlike vs_gbuffer.hlsl, this does NOT project to clip space.
// Instead it outputs world-space position, normal, tangent, and bitangent
// so that the hull shader can compute distance-based tessellation factors,
// and the domain shader can sample the displacement map and then project.

cbuffer Per_object : register(b1)
{
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTransform;
};

struct Vertex_shader_input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

// This struct is the control-point type flowing into the hull shader.
// POSITION is used here as a plain user semantic (not SV_POSITION) since
// the rasteriser only reads SV_POSITION from the last pre-raster stage (DS).
struct Control_point
{
    float3 worldPos : POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

Control_point main(Vertex_shader_input input)
{
    Control_point output;

    float4 worldPos4 = mul(float4(input.position, 1.0f), worldMatrix);
    output.worldPos = worldPos4.xyz;

    output.normal = normalize(mul(input.normal, (float3x3) worldMatrixInvTransform));
    output.tangent = normalize(mul(input.tangent.xyz, (float3x3) worldMatrixInvTransform));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;

    output.uv = input.uv;

    return output;
}
