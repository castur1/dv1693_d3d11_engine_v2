SamplerState samplerLinearWrap : register(s0);

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
    float tessMinFactor;
    float tessMaxFactor;
    float tessMinDistance;
    float tessMaxDistance;
    float displacementScale;
    float3 pad1; // TODO: Normal strength multiplier and texel size I guess
};

Texture2D displacementTexture : register(t2);

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

struct Domain_shader_output {
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

[domain("tri")]
Domain_shader_output main(
    HS_constant_data patchConst,
    float3 barycentric : SV_DomainLocation,
    const OutputPatch<Control_point, 3> patch
) {
    Domain_shader_output output;

    float3 positionWorld = barycentric.x * patch[0].positionWorld + barycentric.y * patch[1].positionWorld + barycentric.z * patch[2].positionWorld;
    float3 normal = barycentric.x * patch[0].normal + barycentric.y * patch[1].normal + barycentric.z * patch[2].normal;
    float3 tangent = barycentric.x * patch[0].tangent + barycentric.y * patch[1].tangent + barycentric.z * patch[2].tangent;
    float3 bitangent = barycentric.x * patch[0].bitangent + barycentric.y * patch[1].bitangent + barycentric.z * patch[2].bitangent;
    float2 uv = barycentric.x * patch[0].uv + barycentric.y * patch[1].uv + barycentric.z * patch[2].uv;

    normal = normalize(normal);
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);
    
    float height = displacementTexture.SampleLevel(samplerLinearWrap, uv, 0).r;
    positionWorld += normal * height * displacementScale;
            
    static const float texelSize = 1.0f / 1024.0f;
    static const float normalStrength = 3.0f;

    float heightLeft = displacementTexture.SampleLevel(samplerLinearWrap, uv + float2(-texelSize, 0), 0).r;
    float heightRight = displacementTexture.SampleLevel(samplerLinearWrap, uv + float2(texelSize, 0), 0).r;
    float heightDown = displacementTexture.SampleLevel(samplerLinearWrap, uv + float2(0, -texelSize), 0).r;
    float heightUp = displacementTexture.SampleLevel(samplerLinearWrap, uv + float2(0, texelSize), 0).r;

    float3 displacedNormal = normalize(float3(
        (heightLeft - heightRight) * normalStrength,
        (heightDown - heightUp) * normalStrength,
        1.0f
    ));
    
    float3x3 tbnMatrix = float3x3(tangent, bitangent, normal);
    normal = normalize(mul(displacedNormal, tbnMatrix));

    output.position = mul(float4(positionWorld, 1.0f), viewProjectionMatrix);

    output.normal = normal;
    output.tangent = tangent;
    output.bitangent = bitangent;
    
    output.uv = uv;

    return output;
}
