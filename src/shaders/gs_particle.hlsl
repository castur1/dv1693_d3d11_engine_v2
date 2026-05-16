cbuffer Per_frame_data : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float perFramePad;
};

cbuffer Particle_visual_data : register(b1) {
    float4 startColor;
    float4 endColor;
    float startSize;
    float endSize;
    float nearPlane;
    float farPlane;
};

struct GS_Input
{
    float3 worldPosition : WORLD_POSITION;
    float age : AGE;
    uint isAlive : IS_ALIVE;
};

struct PS_Input
{
    float4 clipPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float age : AGE;
};

[maxvertexcount(4)]
void main(point GS_Input input[1], inout TriangleStream<PS_Input> stream) {
    if (input[0].isAlive == 0)
        return;

    const float age = input[0].age;
    const float size = lerp(startSize, endSize, age);

    if (size <= 0.0f)
        return;

    const float3 worldPos = input[0].worldPosition;

    const float3 right = float3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    const float3 up = float3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    
    const float3 corners[4] = {
        worldPos + (-right - up) * size,
        worldPos + (right - up) * size,
        worldPos + (-right + up) * size,
        worldPos + (right + up) * size
    };

    const float2 uvs[4] = {
        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f)
    };

    [unroll]
    for (int i = 0; i < 4; ++i) {
        PS_Input output;
        output.clipPosition = mul(float4(corners[i], 1.0f), viewProjectionMatrix);
        output.uv = uvs[i];
        output.age = age;
        stream.Append(output);
    }

    stream.RestartStrip();
}
