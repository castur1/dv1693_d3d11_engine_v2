cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
}

cbuffer Lighting_data : register(b1) {
    float3 ambientColour;
    int directionalLightCount;
    int spotLightCount;
    float pad1[3];
}

struct Directional_light_data {
    float3 direction;
    float intensity;
    float3 colour;
    int castsShadows;
    int shadowSliceIndex;
    float pad2[3];
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

Texture2D<float4> gbufferAlbedo : register(t0);
Texture2D<float4> gbufferNormal : register(t1);
Texture2D<float4> gbufferSpecular : register(t2);
Texture2D<float4> depthBuffer : register(t3);

Texture2DArray<float> shadowMapDirectional : register(t4);
Texture2DArray<float> shadowMapSpot : register(t5);

StructuredBuffer<Directional_light_data> directionalLights : register(t6);
StructuredBuffer<Spot_light_data> spotLights : register(t7);

SamplerComparisonState shadowSampler : register(s1);

RWTexture2D<float4> outputTexture : register(u0);

float3 ReconstructWorldPosition(float2 uv, float depth) {
    float2 ndc = float2(2.0f * uv.x - 1.0f, -2.0f * uv.y + 1.0f);
    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldPos = mul(clipPos, invViewProjectionMatrix);
    return worldPos.xyz / worldPos.w;
}

// TODO: Refactor Blinn-Phong

float SampleShadowDirectional(float3 worldPos, int lightIndex)
{
    Directional_light_data light = directionalLights[lightIndex];

    if (!light.castsShadows || light.shadowSliceIndex < 0)
        return 1.0f;

    float4 lightClip = mul(float4(worldPos, 1.0f), light.viewProjectionMatrix);
    float3 ndc = lightClip.xyz / lightClip.w;

    float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
    float depth = ndc.z;

    if (any(uv < 0.0f) || any(uv > 1.0f) || depth <= 0.0f || depth >= 1.0f)
        return 1.0f;

    return shadowMapDirectional.SampleCmpLevelZero(
        shadowSampler, float3(uv, (float) light.shadowSliceIndex), depth);
}

float SampleShadowSpot(float3 worldPos, int lightIndex)
{
    Spot_light_data light = spotLights[lightIndex];

    if (!light.castsShadows || light.shadowSliceIndex < 0)
        return 1.0f;

    float4 lightClip = mul(float4(worldPos, 1.0f), light.viewProjectionMatrix);
    float3 ndc = lightClip.xyz / lightClip.w;

    float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
    float depth = ndc.z;

    if (any(uv < 0.0f) || any(uv > 1.0f) || depth <= 0.0f || depth >= 1.0f)
        return 1.0f;

    return shadowMapSpot.SampleCmpLevelZero(
        shadowSampler, float3(uv, (float) light.shadowSliceIndex), depth);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    outputTexture.GetDimensions(width, height);

    if (id.x >= width || id.y >= height)
        return;

    int2 pixel = int2(id.xy);

    float depth = depthBuffer[pixel].r;
    if (depth >= 1.0f)
        return;

    float4 albedoSample = gbufferAlbedo[pixel];
    float3 albedoColour = albedoSample.rgb;
    float specularExponent = albedoSample.a * 1000.0f;

    float3 normalV = normalize(gbufferNormal[pixel].rgb * 2.0f - 1.0f);
    float3 specularColour = gbufferSpecular[pixel].rgb;

    float2 uv = (float2(pixel) + 0.5f) / float2(width, height);
    float3 worldPos = ReconstructWorldPosition(uv, depth);
    float3 viewV = normalize(cameraPosition - worldPos);

    float3 colour = ambientColour * albedoColour;

    for (int d = 0; d < directionalLightCount; ++d)
    {
        float3 lightV = normalize(-directionalLights[d].direction);
        float diffuseFactor = saturate(dot(normalV, lightV));

        float shadow = 1.0f;
        if (directionalLights[d].castsShadows && diffuseFactor > 0.0f)
            shadow = SampleShadowDirectional(worldPos, d);

        colour += directionalLights[d].colour * directionalLights[d].intensity
                  * albedoColour * diffuseFactor * shadow;

        if (diffuseFactor > 0.0f && shadow > 0.0f)
        {
            float3 halfV = normalize(lightV + viewV);
            float specularFactor = saturate(dot(normalV, halfV));
            colour += directionalLights[d].colour * directionalLights[d].intensity
                      * specularColour * pow(specularFactor, specularExponent) * shadow;
        }
    }

    for (int s = 0; s < spotLightCount; ++s)
    {
        float3 toLight = spotLights[s].position - worldPos;
        float lightDist = length(toLight);

        if (lightDist >= spotLights[s].range)
            continue;

        float3 lightV = toLight / lightDist;
        float cosTheta = dot(spotLights[s].direction, -lightV);
        float cone = smoothstep(spotLights[s].cosOuterAngle,
                                     spotLights[s].cosInnerAngle, cosTheta);
        if (cone <= 0.0f)
            continue;

        float window = saturate(1.0f - pow(lightDist / spotLights[s].range, 4.0f));
        float attenuation = (window * window) / (lightDist * lightDist + 1.0f);
        float factor = spotLights[s].intensity * attenuation * cone;

        float diffuseFactor = saturate(dot(normalV, lightV));

        float shadow = 1.0f;
        if (spotLights[s].castsShadows && diffuseFactor > 0.0f)
            shadow = SampleShadowSpot(worldPos, s);

        colour += spotLights[s].colour * factor * albedoColour * diffuseFactor * shadow;

        if (diffuseFactor > 0.0f && shadow > 0.0f)
        {
            float3 halfV = normalize(lightV + viewV);
            float specularFactor = saturate(dot(normalV, halfV));
            colour += spotLights[s].colour * factor
                      * specularColour * pow(specularFactor, specularExponent) * shadow;
        }
    }

    outputTexture[pixel] = float4(colour, 1.0f);
}