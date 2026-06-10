cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 invViewMatrix;
    float4x4 projectionMatrix;
    float4x4 invProjectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
};

cbuffer Tessellation_data : register(b3) {
    float tessellationMinFactor;
    float tessellationMaxFactor;
    float tessellationMinDistance;
    float tessellationMaxDistance;
    float displacementScale;
    float normalStrength;
    float2 texelSize;
};

struct Control_point {
    float3 positionWorld : POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

struct HS_constant_data {
    float edgeFactors[3] : SV_TessFactor;
    float insideFactor : SV_InsideTessFactor;
};

float ComputeTessellationFactor(float3 worldPoint) {
    float dist = distance(worldPoint, cameraPosition);
    float t = saturate((dist - tessellationMinDistance) / (tessellationMaxDistance - tessellationMinDistance));
    return lerp(tessellationMaxFactor, tessellationMinFactor, t);
}

HS_constant_data PatchConstant(InputPatch<Control_point, 3> patch) {
    HS_constant_data data;

    float3 edgeMiddle0 = (patch[1].positionWorld + patch[2].positionWorld) * 0.5f;
    float3 edgeMiddle1 = (patch[2].positionWorld + patch[0].positionWorld) * 0.5f;
    float3 edgeMiddle2 = (patch[0].positionWorld + patch[1].positionWorld) * 0.5f;

    data.edgeFactors[0] = ComputeTessellationFactor(edgeMiddle0);
    data.edgeFactors[1] = ComputeTessellationFactor(edgeMiddle1);
    data.edgeFactors[2] = ComputeTessellationFactor(edgeMiddle2);

    data.insideFactor = (data.edgeFactors[0] + data.edgeFactors[1] + data.edgeFactors[2]) / 3.0f;

    return data;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstant")]
[maxtessfactor(64.0f)]
Control_point main(InputPatch<Control_point, 3> patch, uint i : SV_OutputControlPointID) {
    return patch[i];
}
