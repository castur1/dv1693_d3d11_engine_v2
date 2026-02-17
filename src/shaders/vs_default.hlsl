#define MAX_LIGHTS 8

struct Light_data {
    float3 position;
    float intensity;
    float3 direction;
    int type;
    float3 colour;
    float spotLightAngle;
};

cbuffer Per_object : register(b0) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInverseTranspose;
}

cbuffer Per_frame : register(b1) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
}

struct Vertex_shader_input {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct Vertex_shader_output {
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

Vertex_shader_output main(Vertex_shader_input input) {
    Vertex_shader_output output;
    
    float4 position = float4(input.position, 1.0f);
    position = mul(position, worldMatrix);
    
    output.worldPosition = position;
    
    position = mul(position, viewMatrix);
    position = mul(position, projectionMatrix); // TODO: Combine view and projection matrices
    output.position = position;
    
    output.normal = mul(input.normal, (float3x3)worldMatrixInverseTranspose);
    output.uv = input.uv;
    
    return output;
}