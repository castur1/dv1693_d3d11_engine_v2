cbuffer Shadow_pass : register(b0) {
    float4x4 viewProjectionMatrix;
};

cbuffer Per_object : register(b1) {
    float4x4 worldMatrix;
    float4x4 worldMatrixInvTranspose;
};

float4 main(float3 position : POSITION) : SV_POSITION {
    float4 worldPosition = mul(float4(position, 1.0f), worldMatrix);
    return mul(worldPosition, viewProjectionMatrix);
}