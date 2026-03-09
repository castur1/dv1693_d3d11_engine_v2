struct Vertex_shader_output {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Vertex_shader_output main(uint id : SV_VertexID) {
    Vertex_shader_output output;

    float2 uv = float2((id << 1) & 2, id & 2);
    output.uv = uv;
    output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}
