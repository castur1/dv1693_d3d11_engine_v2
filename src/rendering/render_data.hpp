#ifndef RENDER_DATA_HPP
#define RENDER_DATA_HPP

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

// CBuffer
struct Per_frame_data {
    XMFLOAT4X4 viewMatrix;
    XMFLOAT4X4 invViewMatrix;
    XMFLOAT4X4 projectionMatrix;
    XMFLOAT4X4 invProjectionMatrix;
    XMFLOAT4X4 viewProjectionMatrix;
    XMFLOAT4X4 invViewProjectionMatrix;
    XMFLOAT3   cameraPosition;
    float      pad0;
};
static_assert(sizeof(Per_frame_data) % 16 == 0);

// CBuffer
struct Per_object_data {
    XMFLOAT4X4 worldMatrix;
    XMFLOAT4X4 worldMatrixInvTranspose;
};
static_assert(sizeof(Per_object_data) % 16 == 0);

// CBuffer
struct Per_material_data {
    XMFLOAT3 materialDiffuse;
    float    isReflective;
    XMFLOAT3 materialSpecular;
    float    materialSpecularExponent;
};
static_assert(sizeof(Per_material_data) % 16 == 0);

// CBuffer
struct Lighting_data {
    XMFLOAT3 ambientColour;
    int      directionalLightCount;
    int      spotLightCount;
    int      reflectionProbeCount;
    int      hasSkybox;
    float    pad0;
};
static_assert(sizeof(Lighting_data) % 16 == 0);

// CBuffer
struct Particle_compute_data {
    XMFLOAT3 acceleration;
    float    deltaTime;
    UINT     maxParticleCount;
    float    pad0[3]; // TODO: Maybe add maxVelocity or something like that?
};
static_assert(sizeof(Particle_compute_data) % 16 == 0);

// CBuffer
struct Particle_visual_data {
    XMFLOAT4 startColour;
    XMFLOAT4 endColour;
    float    startSize;
    float    endSize;
    float    nearPlane;
    float    farPlane;
};
static_assert(sizeof(Particle_visual_data) % 16 == 0);

// CBuffer
struct Debug_resolve_data {
    int   debugMode;
    float nearPlane;
    float farPlane;
    float pad0;
};
static_assert(sizeof(Debug_resolve_data) % 16 == 0);

// Structured buffer element
struct Directional_light_data {
    XMFLOAT3   direction;
    float      intensity;
    XMFLOAT3   colour;
    int        castsShadows;
    int        shadowSliceIndex;
    float      pad0[3];
    XMFLOAT4X4 viewProjectionMatrix;
};
static_assert(sizeof(Directional_light_data) % 16 == 0);

// Structured buffer element
struct Spot_light_data {
    XMFLOAT3   position;
    float      intensity;
    XMFLOAT3   direction;
    float      range;
    XMFLOAT3   colour;
    float      cosInnerAngle;
    float      cosOuterAngle;
    int        castsShadows; // C++ bool != HLSL bool
    int        shadowSliceIndex;
    float      pad0;
    XMFLOAT4X4 viewProjectionMatrix;
};
static_assert(sizeof(Spot_light_data) % 16 == 0);

// Structured buffer element
struct Reflection_probe_data {
    XMFLOAT3 position;
    float    radius;
    int      slotIndex;
    float    pad0[3];
};
static_assert(sizeof(Reflection_probe_data) % 16 == 0);

#endif