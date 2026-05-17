cbuffer Per_frame : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPosition;
    float pad0;
};

cbuffer Particle_visual : register(b1) {
    float4 startColour;
    float4 endColour;
    float startSize;
    float endSize;
    float nearPlane;
    float farPlane;
};

struct Geometry_shader_input {
    float3 positionWorld : POSITION;
    float age : AGE;
    uint isAlive : ALIVE;
};

struct Geometry_shader_ouput {
    float4 positionClip : SV_POSITION;
    float2 uv : TEXCOORD;
    float age : AGE;
};

[maxvertexcount(4)]
void main(point Geometry_shader_input input[1], inout TriangleStream<Geometry_shader_ouput> stream) {
    if (input[0].isAlive == 0)
        return;

    const float age = input[0].age;
    const float size = lerp(startSize, endSize, age);

    if (size <= 0.0f)
        return;

    const float3 positionWorld = input[0].positionWorld;

    const float3 right = viewMatrix._11_21_31;
    const float3 up = viewMatrix._12_22_32;
    
    const float3 corners[4] = {
        positionWorld + (-right - up) * size,
        positionWorld + ( right - up) * size,
        positionWorld + (-right + up) * size,
        positionWorld + ( right + up) * size
    };

    const float2 uvs[4] = {
        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f)
    };

    [unroll]
    for (int i = 0; i < 4; ++i) {
        Geometry_shader_ouput output;
        
        output.positionClip = mul(float4(corners[i], 1.0f), viewProjectionMatrix);
        output.uv = uvs[i];
        output.age = age;
        
        stream.Append(output);
    }

    stream.RestartStrip();
}
