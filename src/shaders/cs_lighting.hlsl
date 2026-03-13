cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
}

cbuffer Lighting_data : register(b1) {
    float3 directionalLightDirection;
    float directionalLightIntensity;
    float3 directionalLightColour;
    int spotLightCount;
    float3 ambientColour;
    float pad1;
}

struct Spot_light_data {
    float3 position;
    float intensity;
    float3 direction;
    float range;
    float3 colour;
    float cosInnerAngle;
    float cosOuterAngle;
    int castsShadows;
    float pad0[2];
    float4x4 viewProjectionMatrix;
};

StructuredBuffer<Spot_light_data> spotLights : register(t4);

Texture2D<float4> gbufferAlbedo : register(t0);
Texture2D<float4> gbufferNormal : register(t1);
Texture2D<float4> gbufferSpecular : register(t2);
Texture2D<float4> depthBuffer : register(t3);

RWTexture2D<float4> outputTexture : register(u0);

float3 ReconstructWorldPosition(float2 uv, float depth) {
    float2 ndc = float2(2.0f * uv.x - 1.0f, -2.0f * uv.y + 1.0f);
    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldPos = mul(clipPos, invViewProjectionMatrix);
    return worldPos.xyz / worldPos.w;
}

// TODO: Refactor Blinn-Phong

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
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
    
    float2 uv = (pixel + 0.5f) / float2(width, height);
    float3 worldPos = ReconstructWorldPosition(uv, depth);
    
    float3 viewV = normalize(cameraPosition - worldPos);
    
    float3 colour = ambientColour * albedoColour;
    
    if (directionalLightIntensity > 0.0f) {
        float3 lightV = normalize(-directionalLightDirection);
        float diffuseFactor = saturate(dot(normalV, lightV)); 
        colour += directionalLightColour * directionalLightIntensity * albedoColour * diffuseFactor;
        
        if (diffuseFactor > 0.0f) {
            float3 halfV = normalize(lightV + viewV);
            float specularFactor = saturate(dot(normalV, halfV));
            colour += directionalLightColour * directionalLightIntensity * specularColour * pow(specularFactor, specularExponent);
        }
    }
    
    for (int i = 0; i < spotLightCount; ++i) {
        float3 lightV = spotLights[i].position - worldPos;
        float lightVLength = length(lightV);
        
        if (lightVLength >= spotLights[i].range)
            continue;
        
        lightV /= lightVLength;
        
        float cosTheta = dot(spotLights[i].direction, -lightV);
        float cone = smoothstep(spotLights[i].cosOuterAngle, spotLights[i].cosInnerAngle, cosTheta);

        if (cone <= 0.0f)
            continue;
                
        float window = saturate(1.0f - pow(lightVLength / spotLights[i].range, 4));
        float attenuation = (window * window) / (lightVLength * lightVLength + 1.0f);
        float factor = spotLights[i].intensity * attenuation * cone;
        
        float diffuseFactor = saturate(dot(normalV, lightV));
        colour += spotLights[i].colour * factor * albedoColour * diffuseFactor;
        
        if (diffuseFactor > 0.0f) {
            float3 halfV = normalize(lightV + viewV);
            float specularFactor = saturate(dot(normalV, halfV));
            colour += spotLights[i].colour * factor * specularColour * pow(specularFactor, specularExponent);
        }
    }
        
    outputTexture[pixel] = float4(colour, 1.0f);
}