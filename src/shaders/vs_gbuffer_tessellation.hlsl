cbuffer Per_object : register(b1) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTransform;
};

struct Vertex_shader_input {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct Control_point {
    float3 position : POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

Control_point main(Vertex_shader_input input) {
    Control_point output;

    output.position = mul(float4(input.position, 1.0f), worldMatrix).xyz;

    output.normal = normalize(mul(input.normal, (float3x3)worldMatrixInvTransform));
    output.tangent = normalize(mul(input.tangent.xyz, (float3x3)worldMatrixInvTransform));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;

    output.uv = input.uv;

    return output;
}
