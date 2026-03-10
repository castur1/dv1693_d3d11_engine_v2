struct Vertex_shader_output {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Generates a fullscreen triangle
// 0 -> uv = (0, 0), pos = (-1,  1)
// 1 -> uv = (2, 0), pos = ( 3,  1)
// 2 -> uv = (0, 2), pos = (-1, -3)

Vertex_shader_output main(uint id : SV_VertexID) {
    Vertex_shader_output output;

    float2 uv = float2((id << 1) & 2, id & 2);
    output.uv = uv;
    output.position = float4(2 * uv.x - 1, -2 * uv.y + 1, 0.0f, 1.0f);

    return output;
}
