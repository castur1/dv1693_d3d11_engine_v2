cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
}

cbuffer Per_object : register(b1) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTransform;
};

cbuffer Per_material : register(b2) {
    float3 materialAmbient;
    int isReflective;
    float3 materialDiffuse;
    float pad1;
    float3 materialSpecular;
    float materialSpecularExponent;
}

cbuffer Lighting_data : register(b3) {
    float3 ambientColour;
    int directionalLightCount;
    int spotLightCount;
    int probeCount;
    float2 pad;
};

struct Directional_light_data {
    float3 direction;
    float intensity;
    float3 colour;
    int castsShadows;
    int shadowSliceIndex;
    float3 pad;
    float4x4 viewProjectionMatrix;
};

struct Spot_light_data {
    float3 position;
    float intensity;
    float3 direction;
    float range;
    float3 colour;
    float cosInnerAngle;
    float cosOuterAngle;
    int castsShadows;
    int shadowSliceIndex;
    float pad3;
    float4x4 viewProjectionMatrix;
};

StructuredBuffer<Directional_light_data> g_directionalLights : register(t0);
StructuredBuffer<Spot_light_data> spotLights : register(t1);

Texture2D g_diffuse : register(t2);
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
    
    // CONTINUE HERE!

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