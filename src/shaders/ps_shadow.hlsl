SamplerState samplerLinearWrap : register(s0);

Texture2D diffuseTexture : register(t0);

struct Pixel_shader_input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

void main(Pixel_shader_input input) {
    if (diffuseTexture.Sample(samplerLinearWrap, input.uv).a < 0.5f)
        discard;
}