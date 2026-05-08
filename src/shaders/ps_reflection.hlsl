struct Directional_light_data
{
    float3 direction;
    float intensity;
    float3 colour;
    int castsShadows;
    int shadowSliceIndex;
    float3 pad;
    float4x4 viewProjectionMatrix;
};

cbuffer Lighting_data : register(b1)
{
    float3 ambientColour;
    int directionalLightCount;
    int spotLightCount;
    int probeCount;
    float2 pad;
};

StructuredBuffer<Directional_light_data> g_directionalLights : register(t0);

Texture2D g_diffuse : register(t1);
SamplerState g_sampler : register(s0);

struct PS_Input
{
    float4 posCS : SV_Position;
    float3 posWS : POSITION;
    float3 normalWS : NORMAL;
    float2 uv : TEXCOORD;
};

float4 main(PS_Input input) : SV_Target
{
    float3 N = normalize(input.normalWS);
    float3 albedo = g_diffuse.Sample(g_sampler, input.uv).rgb;

    float3 colour = ambientColour * albedo;

    for (int i = 0; i < directionalLightCount; ++i)
    {
        Directional_light_data light = g_directionalLights[i];
        float3 L = normalize(-light.direction);
        float NdotL = saturate(dot(N, L));
        colour += albedo * light.colour * light.intensity * NdotL;
    }

    return float4(colour, 1.0f);
}