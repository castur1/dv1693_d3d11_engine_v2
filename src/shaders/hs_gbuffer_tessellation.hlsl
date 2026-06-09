// hs_gbuffer.hlsl
//
// Hull shader for the tessellated G-buffer pass.
//
// Constant function (PatchConstant):
//   Computes per-edge and interior tessellation factors based on the
//   distance from each edge's midpoint to the camera. Closer = more
//   tessellation. Uses a smooth linear falloff between minDistance
//   (max factor) and maxDistance (min factor).
//
// Control-point function (main):
//   Pure pass-through — all interpolation happens in the domain shader.

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
    float displacementScale; // per-material, written by C++ before each draw
    float3 pad1;
};

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

// ---------------------------------------------------------------------------
// Compute a tessellation factor for a single world-space point (typically an
// edge midpoint). Factor is max at minDistance and falls to min at maxDistance.
// ---------------------------------------------------------------------------
float ComputeTessFactor(float3 worldPoint)
{
    float dist = distance(worldPoint, cameraPosition);
    float t = saturate((dist - tessMinDistance) /
                           max(tessMaxDistance - tessMinDistance, 0.001f));
    return lerp(tessMaxFactor, tessMinFactor, t);
}

// ---------------------------------------------------------------------------
// Patch-constant function — runs once per triangle patch.
// Edge i is OPPOSITE vertex i (D3D11 convention for tri patches):
//   edge[0] = opposite patch[0] = midpoint of patch[1]-patch[2]
//   edge[1] = opposite patch[1] = midpoint of patch[2]-patch[0]
//   edge[2] = opposite patch[2] = midpoint of patch[0]-patch[1]
// ---------------------------------------------------------------------------
HS_Constant_data PatchConstant(
    InputPatch<Control_point, 3> patch,
    uint patchID : SV_PrimitiveID)
{
    HS_Constant_data data;

    float3 e0mid = (patch[1].worldPos + patch[2].worldPos) * 0.5f;
    float3 e1mid = (patch[2].worldPos + patch[0].worldPos) * 0.5f;
    float3 e2mid = (patch[0].worldPos + patch[1].worldPos) * 0.5f;

    data.edgeTess[0] = ComputeTessFactor(e0mid);
    data.edgeTess[1] = ComputeTessFactor(e1mid);
    data.edgeTess[2] = ComputeTessFactor(e2mid);

    // Interior factor: average of edge factors gives smooth gradients
    data.insideTess = (data.edgeTess[0] + data.edgeTess[1] + data.edgeTess[2]) / 3.0f;

    return data;
}

// ---------------------------------------------------------------------------
// Control-point function — runs once per control point per patch.
// Pure pass-through; all attribute interpolation happens in the DS.
// ---------------------------------------------------------------------------
[domain("tri")]
[partitioning("fractional_odd")] // smooth, crack-free transitions
[outputtopology("triangle_cw")] // matches existing left-handed CW winding
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstant")]
[maxtessfactor(64.0f)]
Control_point main(
    InputPatch<Control_point, 3> patch,
    uint i : SV_OutputControlPointID,
    uint patchID : SV_PrimitiveID)
{
    return patch[i];
}
