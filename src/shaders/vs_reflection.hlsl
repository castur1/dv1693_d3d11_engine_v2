cbuffer Per_frame_data : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
};

cbuffer Per_object_data : register(b1)
{
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTranspose;
};

struct VS_Input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 tangent : TANGENT;
};

struct VS_Output
{
    float4 posCS : SV_Position;
    float3 posWS : POSITION;
    float3 normalWS : NORMAL;
    float2 uv : TEXCOORD;
};

VS_Output main(VS_Input input)
{
    VS_Output output;
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.posWS = worldPos.xyz;
    output.posCS = mul(worldPos, viewProjectionMatrix);
    output.normalWS = normalize(mul(input.normal, (float3x3) worldMatrixInvTranspose));
    output.uv = input.uv;
    return output;
}