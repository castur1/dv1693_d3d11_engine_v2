cbuffer Per_material : register(b2) {
    float3 materialAmbient;
    float pad0;
    float3 materialDiffuse;
    float pad1;
    float3 materialSpecular;
    float materialSpecularExponent;
};

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);

SamplerState linearSampler : register(s0);

struct Pixel_shader_input {
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

struct Pixel_shader_output {
    float4 albedo : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 specular : SV_TARGET2;
};

Pixel_shader_output main(Pixel_shader_input input) {
    Pixel_shader_output output;

    float3 albedo = materialDiffuse * diffuseTexture.Sample(linearSampler, input.uv).rgb;
    output.albedo = float4(albedo, materialSpecularExponent / 1000.0f);

    float3 normal = normalTexture.Sample(linearSampler, input.uv).rgb * 2.0f - 1.0f;
    float3x3 tbnMatrix = float3x3(
        normalize(input.tangent),
        normalize(input.bitangent),
        normalize(input.normal)
    );
    output.normal = float4(normalize(mul(normal, tbnMatrix)) * 0.5f + 0.5f, 0.0f);

    output.specular = float4(materialSpecular, 0.0f);

    return output;
}
