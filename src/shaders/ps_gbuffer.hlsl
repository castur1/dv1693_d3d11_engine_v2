cbuffer Per_material : register(b2) {
    float3 materialAmbient;
    float pad0;
    float3 materialDiffuse;
    float pad1;
    float3 materialSpecular;
    float materialSpecularExponent;
};

Texture2D diffuseTexture : register(t0);

SamplerState linearSampler : register(s0);

struct Pixel_shader_input {
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

struct Pixel_shader_output {
    float4 albedo : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 specular : SV_TARGET2;
};

Pixel_shader_output main(Pixel_shader_input input)
{
    Pixel_shader_output output;

    float3 albedo = materialDiffuse * diffuseTexture.Sample(linearSampler, input.uv).rgb;
    output.albedo = float4(albedo, materialSpecularExponent / 1000.0f);

    float3 normal = normalize(input.normal);
    output.normal = float4(normal * 0.5f + 0.5f, 0.0f);

    output.specular = float4(materialSpecular, 0.0f);

    return output;
}
