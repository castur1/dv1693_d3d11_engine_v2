SamplerState samplerLinearWrap : register(s0);

Texture2D diffuseTexture : register(t0);

#define MAX_LIGHTS 8

struct Light_data {
    float3 position;
    float intensity;
    float3 direction;
    int type;
    float3 colour;
    float spotLightCosHalfAngle;
};

cbuffer Per_object : register(b0) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInverseTranspose; 
}

cbuffer Per_frame : register(b1) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
}

cbuffer Per_material_data : register(b3) {
    float3 materialAmbient;
    float pad1;
    float3 materialDiffuse;
    float pad2;
    float3 materialSpecular;
    float materialSpecularExponent;
};

cbuffer Lighting : register(b2) {
    float3 cameraPosition;
    int lightCount;

    float3 ambientLight;
    float pad0;

    Light_data lights[MAX_LIGHTS];
};

struct Pixel_shader_input {
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

float4 main(Pixel_shader_input input) : SV_TARGET {
    float4 textureColour = diffuseTexture.Sample(samplerLinearWrap, input.uv);
        
    float3 normalV = normalize(input.normal);
    float3 viewV = normalize(cameraPosition - input.worldPosition);

    float3 ambient  = ambientLight;
    float3 diffuse  = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < lightCount; ++i) {
        Light_data lightSource = lights[i];
        
        float3 lightV;
        float attenuation;
        
        // Directional
        if (lightSource.type == 0) { 
            lightV = normalize(-lightSource.direction);
            attenuation = 1.0f;
        }
        // Point
        else if (lightSource.type == 1) {
            lightV = lightSource.position - input.worldPosition;
            float distance = length(lightV);
            lightV = normalize(lightV);

            attenuation = 1 / (distance * distance + 0.001f);
        }
        // Spot
        else {
            lightV = lightSource.position - input.worldPosition;
            float distance = length(lightV);
            lightV = normalize(lightV);

            attenuation = 1 / (distance * distance + 0.001f);
            
            float spotFactor = dot(-lightV, normalize(lightSource.direction));
            if (spotFactor < lightSource.spotLightCosHalfAngle)
                attenuation = 0.0f;
        }
        
        float3 lightStrength = lightSource.colour * lightSource.intensity * attenuation;
        
        float diffuseFactor = saturate(dot(normalV, lightV));
        diffuse += diffuseFactor * lightStrength;

        if (diffuseFactor > 0) {
            float3 reflectionV = reflect(-lightV, normalV);

            float specularFactor = pow(saturate(dot(reflectionV, viewV)), materialSpecularExponent);
            specular += specularFactor * lightStrength * materialSpecular;
        }
    }
    
    ambient *= materialAmbient;
    diffuse *= materialDiffuse;
    
    float3 finalColour = (ambient + diffuse) * textureColour.rgb + specular;
    
    finalColour = pow(finalColour, 1.0 / 2.2); // Gamma correction
    
    return float4(finalColour, 1.0f);
}