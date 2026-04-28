#ifndef MODEL_HPP
#define MODEL_HPP

#include "resources/asset_manager.hpp"

#include <DirectXCollision.h>

#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
#include <xhash>

using namespace DirectX;

#define SafeRelease(obj) do { if (obj) (obj)->Release(); (obj) = nullptr; } while (0)

struct Vertex {
    XMFLOAT3 position{};
    XMFLOAT3 normal{};
    XMFLOAT2 uv{};

    XMFLOAT4 tangent{};

    // For std::unordered_map
    // We don't consider tangent since it is computed after vertex deduplication
    bool operator==(const Vertex &other) const {
        if (position.x != other.position.x) return false;
        if (position.y != other.position.y) return false;
        if (position.z != other.position.z) return false;

        if (normal.x != other.normal.x) return false;
        if (normal.y != other.normal.y) return false;
        if (normal.z != other.normal.z) return false;

        if (uv.x != other.uv.x) return false;
        if (uv.y != other.uv.y) return false;

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

            combine(v.position.x);
            combine(v.position.y);
            combine(v.position.z);

            combine(v.normal.x);
            combine(v.normal.y);
            combine(v.normal.z);

            combine(v.uv.x);
            combine(v.uv.y);

            return seed;
        }
    };
}

struct Pipeline_state {
    ID3D11VertexShader *vertexShader = nullptr;
    ID3D11PixelShader *pixelShader = nullptr;
    ID3D11InputLayout *inputLayout = nullptr;

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

    int width = 0;
    int height = 0;

    ~Texture2D() {
        SafeRelease(this->shaderResourceView);
    }
};

struct Material {
    AssetHandle<Pipeline_state> pipelineState;

    AssetHandle<Texture2D> diffuseTexture;
    AssetHandle<Texture2D> normalTexture;

    XMFLOAT3 ambientColour = {1.0f, 1.0f, 1.0f};
    XMFLOAT3 diffuseColour = {1.0f, 1.0f, 1.0f};
    XMFLOAT3 specularColour = {1.0f, 1.0f, 1.0f};
    float specularExponent = 32.0f;
};

struct Model {
    ID3D11Buffer *vertexBuffer = nullptr;
    ID3D11Buffer *indexBuffer = nullptr;

    BoundingBox localBounds;

    struct Mesh {
        UINT indexCount;
        UINT startIndex;
        UINT baseVertex;
    };

    struct Sub_model {
        Mesh mesh;
        AssetHandle<Material> material;
        BoundingBox localBounds;
    };

    std::vector<Sub_model> subModels;

    ~Model() {
        SafeRelease(this->indexBuffer);
        SafeRelease(this->vertexBuffer);
    }
};

#endif