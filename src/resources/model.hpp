#ifndef MODEL_HPP
#define MODEL_HPP

#include "resources/asset_manager.hpp"

#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
#include <xhash>

using namespace DirectX;

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

struct Vertex {
    float position[3] = {};
    float normal[3]   = {};
    float uv[2]       = {};

    float tangent[4]  = {};

    // For std::unordered_map
    // We don't consider tangent since it is computed after vertex deduplication
    bool operator==(const Vertex& other) const {
        if (position[0] != other.position[0]) return false;
        if (position[1] != other.position[1]) return false;
        if (position[2] != other.position[2]) return false;

        if (normal[0] != other.normal[0]) return false;
        if (normal[1] != other.normal[1]) return false;
        if (normal[2] != other.normal[2]) return false;

        if (uv[0] != other.uv[0]) return false;
        if (uv[1] != other.uv[1]) return false;

        return true;
    }
};

// https://stackoverflow.com/questions/19195183/how-to-properly-hash-the-custom-struct
namespace std {
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex &v) const {
            size_t seed = 0;
            std::hash<float> hasher;
            auto combine = [&](float f) {
                seed ^= hasher(f) + (seed << 6) + (seed >> 2) + 0x9e3779b9;
            };

            combine(v.position[0]);
            combine(v.position[1]);
            combine(v.position[2]);

            combine(v.normal[0]);
            combine(v.normal[1]);
            combine(v.normal[2]);

            combine(v.uv[0]);
            combine(v.uv[1]);

            return seed;
        }
    };
}

struct Pipeline_state {
    ID3D11VertexShader *vertexShader = nullptr;
    ID3D11PixelShader  *pixelShader  = nullptr;
    ID3D11InputLayout  *inputLayout  = nullptr;

    // TODO: Fixed-function states?

    D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    ~Pipeline_state() {
        SafeRelease(this->inputLayout);
        SafeRelease(this->pixelShader);
        SafeRelease(this->vertexShader);
    }
};

struct Texture2D {
    ID3D11ShaderResourceView *shaderResourceView = nullptr;

    int width  = 0;
    int height = 0;

    ~Texture2D() {
        SafeRelease(this->shaderResourceView);
    }
};

struct Material {
    AssetHandle<Pipeline_state> pipelineState;

    AssetHandle<Texture2D> diffuseTexture;

    XMFLOAT3 ambientColour  = {1.0f, 1.0f, 1.0f};
    XMFLOAT3 diffuseColour  = {1.0f, 1.0f, 1.0f};
    XMFLOAT3 specularColour = {1.0f, 1.0f, 1.0f};
    float specularExponent  = 32.0f;
};

struct Model {
    ID3D11Buffer *vertexBuffer = nullptr;
    ID3D11Buffer *indexBuffer  = nullptr;

    struct Mesh {
        UINT indexCount;
        UINT startIndex;
        UINT baseVertex;
    };

    struct Sub_model {
        Mesh mesh;
        AssetHandle<Material> material;
    };

    std::vector<Sub_model> subModels;

    ~Model() {
        SafeRelease(this->indexBuffer);
        SafeRelease(this->vertexBuffer);
    }
};

#endif