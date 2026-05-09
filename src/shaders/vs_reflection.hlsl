cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
};

cbuffer Per_object : register(b1) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTranspose;
};

struct Vertex_shader_input {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 tangent : TANGENT;
};

struct Vertex_shader_output {
    float4 positionClip : SV_POSITION;
    float3 positionWorld : POSITION;
    float3 normalWorld : NORMAL;
    float2 uv : TEXCOORD;
};

Vertex_shader_output main(Vertex_shader_input input) {
    Vertex_shader_output output;
    
    float4 positionWorld = mul(float4(input.position, 1.0f), worldMatrix);
    output.positionWorld = positionWorld.xyz;
    output.positionClip = mul(positionWorld, viewProjectionMatrix);
    
    output.normalWorld = normalize(mul(input.normal, (float3x3)worldMatrixInvTranspose));
    
    output.uv = input.uv;
    
    return output;
}