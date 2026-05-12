cbuffer Shadow_pass : register(b0) {
    float4x4 viewProjectionMatrix;
};

cbuffer Per_object : register(b1) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTranspose;
};

struct Vertex_shader_input {
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct Vertex_shader_output {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Vertex_shader_output main(Vertex_shader_input input) {
    Vertex_shader_output output;
    
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(worldPosition, viewProjectionMatrix);
    
    output.uv = input.uv;
    
    return output;
}