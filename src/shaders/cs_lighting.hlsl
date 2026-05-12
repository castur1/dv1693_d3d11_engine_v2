SamplerState samplerLinearWrap : register(s0);
SamplerComparisonState samplerShadow : register(s1);

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
    int reflectionProbeCount;
    int hasSkybox;
    float pad1;
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

struct Reflection_probe_data {
    float3 position;
    float radius;
    int slotIndex;
    float3 pad4;
};

Texture2D<float4> gbufferAlbedo : register(t0);
Texture2D<float4> gbufferNormal : register(t1);
Texture2D<float4> gbufferSpecular : register(t2);
Texture2D<float4> depthBuffer : register(t3);

Texture2DArray<float> shadowMapDirectional : register(t4);
Texture2DArray<float> shadowMapSpot : register(t5);

StructuredBuffer<Directional_light_data> directionalLights : register(t6);
StructuredBuffer<Spot_light_data> spotLights : register(t7);

TextureCubeArray reflectionCubeMap : register(t8);
StructuredBuffer<Reflection_probe_data> reflectionProbes : register(t9);
TextureCube skybox : register(t10);

RWTexture2D<float4> outputTexture : register(u0);

float3 ReconstructWorldPosition(float2 uv, float depth) {
    float2 ndc = float2(2.0f * uv.x - 1.0f, -2.0f * uv.y + 1.0f);
    float4 positionClip = float4(ndc, depth, 1.0f);
    float4 positionWorld = mul(positionClip, invViewProjectionMatrix);
    return positionWorld.xyz / positionWorld.w;
}

float3 SampleReflectionProbe(float3 reflectV, float3 positionWorld, uint2 pixel) {
    float closestDistanceSq = 999999.0f;
    int indexOfClosest = -1;

    for (int i = 0; i < reflectionProbeCount; ++i) {
        float3 deltaV = positionWorld - reflectionProbes[i].position;
            
        float distanceToCentreSq = dot(deltaV, deltaV);
        float radiusSq = reflectionProbes[i].radius * reflectionProbes[i].radius;
            
        if (distanceToCentreSq <= radiusSq && (distanceToCentreSq < closestDistanceSq || indexOfClosest == -1)) {
            indexOfClosest = i;
            closestDistanceSq = distanceToCentreSq;
        }
    }
        
    if (indexOfClosest != -1)
        return reflectionCubeMap.SampleLevel(samplerLinearWrap, float4(reflectV, reflectionProbes[indexOfClosest].slotIndex), 0.0f).rgb;
    else if (hasSkybox)
        return skybox.SampleLevel(samplerLinearWrap, reflectV, 0.0f).rgb;
    
    return outputTexture[pixel].rgb;
}

float SampleShadowDirectional(float3 positionWorld, int lightIndex) {
    Directional_light_data light = directionalLights[lightIndex];

    if (!light.castsShadows || light.shadowSliceIndex < 0)
        return 1.0f;

    float4 lightClip = mul(float4(positionWorld, 1.0f), light.viewProjectionMatrix);
    float3 ndc = lightClip.xyz / lightClip.w;

    float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
    float depth = ndc.z;
    
    if (any(uv < 0.0f) || any(uv > 1.0f) || depth < 0.0f || depth > 1.0f)
        return 1.0f;

    return shadowMapDirectional.SampleCmpLevelZero(
        samplerShadow, 
        float3(uv, light.shadowSliceIndex), 
        depth
    );
}

float SampleShadowSpot(float3 positionWorld, int lightIndex) {
    Spot_light_data light = spotLights[lightIndex];

    if (!light.castsShadows || light.shadowSliceIndex < 0)
        return 1.0f;

    float4 lightClip = mul(float4(positionWorld, 1.0f), light.viewProjectionMatrix);
    float3 ndc = lightClip.xyz / lightClip.w;

    float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
    float depth = ndc.z;
    
    if (any(uv < 0.0f) || any(uv > 1.0f) || depth < 0.0f || depth > 1.0f)
        return 1.0f;

    return shadowMapSpot.SampleCmpLevelZero(
        samplerShadow, 
        float3(uv, light.shadowSliceIndex), 
        depth
    );
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint width, height;
    outputTexture.GetDimensions(width, height);

    if (id.x >= width || id.y >= height)
        return;

    int2 pixel = int2(id.xy);
    
    float depth = depthBuffer[pixel].r;
    
    float2 uv = (float2(pixel) + 0.5f) / float2(width, height);
    float3 positionWorld = ReconstructWorldPosition(uv, depth);
    float3 viewV = normalize(cameraPosition - positionWorld);

    if (depth >= 1.0f) {
        if (hasSkybox)
            outputTexture[pixel] = float4(1.2f * skybox.SampleLevel(samplerLinearWrap, -viewV, 0).rgb, 1.0f);
        
        return;
    }

    float4 albedoSample = gbufferAlbedo[pixel];
    float3 albedoColour = albedoSample.rgb;
    float specularExponent = albedoSample.a * 1000.0f;

    float4 normalSample = gbufferNormal[pixel];
    float3 normalV = normalize(normalSample.rgb * 2.0f - 1.0f);
    bool isReflective = normalSample.a > 0.5f;
    
    float3 specularColour = gbufferSpecular[pixel].rgb;
    
    if (isReflective) {
        float3 reflectV = reflect(-viewV, normalV);
        
        float3 environmentColour = SampleReflectionProbe(reflectV, positionWorld, pixel);
        outputTexture[pixel] = float4(albedoColour * environmentColour, 1.0f);
        
        return;
    }
    
    float3 ambient = ambientColour;
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < directionalLightCount; ++i) {
        float3 lightV = normalize(-directionalLights[i].direction);
        
        float diffuseFactor = dot(normalV, lightV);
        if (diffuseFactor <= 0.0f)
            continue;

        float shadowFactor = 1.0f;
        if (directionalLights[i].castsShadows)
            shadowFactor = SampleShadowDirectional(positionWorld, i);
        
        if (shadowFactor <= 0.0f)
            continue;
        
        float3 radiance = directionalLights[i].colour * directionalLights[i].intensity * shadowFactor;

        diffuse += radiance * diffuseFactor;

        float3 halfV = normalize(lightV + viewV);
        float specularFactor = saturate(dot(normalV, halfV));
        specular += radiance * pow(specularFactor, specularExponent);
    }

    for (int i = 0; i < spotLightCount; ++i) {
        float3 lightV = spotLights[i].position - positionWorld;
        float distance = length(lightV);

        if (distance >= spotLights[i].range)
            continue;
        
        lightV /= distance;
        
        float diffuseFactor = dot(normalV, lightV);
        if (diffuseFactor <= 0.0)
            continue;
        
        float t = dot(spotLights[i].direction, -lightV);
        float coneFactor = smoothstep(spotLights[i].cosOuterAngle, spotLights[i].cosInnerAngle, t);
        if (coneFactor <= 0.0f)
            continue;
        
        float shadowFactor = 1.0f;
        if (spotLights[i].castsShadows)
            shadowFactor = SampleShadowSpot(positionWorld, i);
        
        if (shadowFactor <= 0.0f)
            continue;
        
        float window = saturate(1.0f - pow(distance / spotLights[i].range, 4.0f));
        float attenuation = (window * window) / (distance * distance + 1.0f);
        
        float3 radiance = spotLights[i].colour * spotLights[i].intensity * attenuation * coneFactor;
        
        diffuse += radiance * diffuseFactor;

        float3 halfV = normalize(lightV + viewV);
        float specularFactor = saturate(dot(normalV, halfV));
        specular += radiance * pow(specularFactor, specularExponent);
    }
        
    float3 finalColour = (ambient + diffuse) * albedoColour + specular * specularColour;
    outputTexture[pixel] = float4(finalColour, 1.0f);
}