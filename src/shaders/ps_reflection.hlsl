SamplerState samplerLinearWrap : register(s0);

cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
}

cbuffer Per_material : register(b1) {
    float3 materialDiffuse;
    float isReflective;
    float3 materialSpecular;
    float materialSpecularExponent;
}

cbuffer Lighting_data : register(b2) {
    float3 ambientColour;
    int directionalLightCount;
    int spotLightCount;
    int probeCount;
    float2 pad1;
};

struct Directional_light_data {
    float3 direction;
    float intensity;
    float3 colour;
    int castsShadows;
    int shadowSliceIndex;
    float3 pad2;
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

StructuredBuffer<Directional_light_data> directionalLights : register(t0);
StructuredBuffer<Spot_light_data> spotLights : register(t1);

Texture2D diffuseTexture : register(t2);

struct Pixel_shader_input {
    float4 positionClip : SV_POSITION;
    float3 positionWorld : POSITION;
    float3 normalWorld : NORMAL;
    float2 uv : TEXCOORD;
};

float4 main(Pixel_shader_input input) : SV_TARGET {
    float3 albedo = diffuseTexture.Sample(samplerLinearWrap, input.uv).rgb;
    
    float3 normalV = normalize(input.normalWorld);
    float3 viewV = normalize(cameraPosition - input.positionWorld);
    
    float3 ambient = ambientColour;
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < directionalLightCount; ++i) {
        float3 lightV = -directionalLights[i].direction;
        
        float diffuseFactor = dot(normalV, lightV);
        if (diffuseFactor <= 0.0f)
            continue;

        float3 radiance = directionalLights[i].colour * directionalLights[i].intensity;

        diffuse += radiance * diffuseFactor;
        
        float3 halfV = normalize(lightV + viewV);
        float specularFactor = pow(saturate(dot(normalV, halfV)), materialSpecularExponent);
        specular += radiance * specularFactor;
    }

    for (int i = 0; i < spotLightCount; ++i) {
        float3 lightV = spotLights[i].position - input.positionWorld;
        float distance = length(lightV);

        if (distance >= spotLights[i].range)
            continue;

        lightV /= distance;
        
        float diffuseFactor = dot(normalV, lightV);
        if (diffuseFactor <= 0.0f)
            continue;
        
        float t = dot(spotLights[i].direction, -lightV);
        float coneFactor = smoothstep(spotLights[i].cosOuterAngle, spotLights[i].cosInnerAngle, t);
        if (coneFactor <= 0.0f)
            continue;

        float window = saturate(1.0f - pow(distance / spotLights[i].range, 4.0f));
        float attenuation = (window * window) / (distance * distance + 1.0f);

        float3 radiance = spotLights[i].colour * spotLights[i].intensity * attenuation * coneFactor;

        diffuse += radiance * diffuseFactor;
        
        float3 halfV = normalize(lightV + viewV);
        float specularFactor = pow(saturate(dot(normalV, halfV)), materialSpecularExponent);
        specular += radiance * specularFactor;
    }
    
    diffuse *= materialDiffuse;
    specular *= materialSpecular;
    
    float3 finalColour = (ambient + diffuse) * albedo + specular;
    return float4(finalColour, 1.0f);
}