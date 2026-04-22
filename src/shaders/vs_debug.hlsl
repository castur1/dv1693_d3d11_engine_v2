cbuffer Per_frame : register(b0) {
    float4x4 viewProjectionMatrix;
};

struct Vertex_shader_input {
    float3 position : POSITION;
    float4 colour : COLOUR;
};

struct Vertex_shader_output {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

Vertex_shader_output main(Vertex_shader_input input) {
    Vertex_shader_output output;
    
    output.position = mul(float4(input.position, 1.0f), viewProjectionMatrix);
    output.colour = input.colour;
    
    return output;
}