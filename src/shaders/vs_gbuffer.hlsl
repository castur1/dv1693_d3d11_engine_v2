cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
};

cbuffer Per_object : register(b1) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTransform;
};

struct Vertex_shader_input {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct Vertex_shader_output {
    float4 positionClip : SV_POSITION;
    float3 positionWorld : TEXCOORD0;
    float3 normalWorld : TEXCOORD1;
    float2 uv : TEXCOORD2;
};

Vertex_shader_output main(Vertex_shader_input input) {
    Vertex_shader_output output;

    float4 position = float4(input.position, 1.0f);
    position = mul(position, worldMatrix);
    
    output.positionWorld = position.xyz;
    output.positionClip = mul(position, viewProjectionMatrix);
    output.normalWorld = mul(input.normal, (float3x3)worldMatrixInvTransform);
    output.uv = input.uv;

    return output;
}
