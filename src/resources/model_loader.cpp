#include "model_loader.hpp"
#include "core/logging.hpp"
#include "resources/asset_manager.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

#include <unordered_map>

void ModelLoader::ComputeTangents(std::vector<Vertex> &vertices, const std::vector<UINT> &indices) {
    std::vector<XMFLOAT3> tangents(vertices.size(), {0.0f, 0.0f, 0.0f});
    std::vector<XMFLOAT3> bitangents(vertices.size(), {0.0f, 0.0f, 0.0f});

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        UINT i0 = indices[i];
        UINT i1 = indices[i + 1];
        UINT i2 = indices[i + 2];

        const Vertex &v0 = vertices[i0];
        const Vertex &v1 = vertices[i1];
        const Vertex &v2 = vertices[i2];

        XMVECTOR p0 = XMLoadFloat3(&v0.position);
        XMVECTOR p1 = XMLoadFloat3(&v1.position);
        XMVECTOR p2 = XMLoadFloat3(&v2.position);

        XMVECTOR e1 = p1 - p0;
        XMVECTOR e2 = p2 - p0;

        float du1 = v1.uv.x - v0.uv.x;
        float dv1 = v1.uv.y - v0.uv.y;
        float du2 = v2.uv.x - v0.uv.x;
        float dv2 = v2.uv.y - v0.uv.y;

        // https://en.wikipedia.org/wiki/Cramer%27s_rule
        float det = du1 * dv2 - du2 * dv1;
        if (fabsf(det) < 1e-6f)
            continue;

        XMVECTOR t = (e1 * dv2 - e2 * dv1) / det;
        XMVECTOR b = (e2 * du1 - e1 * du2) / det;

        XMFLOAT3 tf;
        XMStoreFloat3(&tf, t);
        XMFLOAT3 bf;
        XMStoreFloat3(&bf, b);

        tangents[i0].x += tf.x;
        tangents[i0].y += tf.y;
        tangents[i0].z += tf.z;

        bitangents[i0].x += bf.x;
        bitangents[i0].y += bf.y;
        bitangents[i0].z += bf.z;

        tangents[i1].x += tf.x;
        tangents[i1].y += tf.y;
        tangents[i1].z += tf.z;

        bitangents[i1].x += bf.x;
        bitangents[i1].y += bf.y;
        bitangents[i1].z += bf.z;

        tangents[i2].x += tf.x;
        tangents[i2].y += tf.y;
        tangents[i2].z += tf.z;

        bitangents[i2].x += bf.x;
        bitangents[i2].y += bf.y;
        bitangents[i2].z += bf.z;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        Vertex &v = vertices[i];

        XMVECTOR n = XMLoadFloat3(&v.normal);
        XMVECTOR t = XMLoadFloat3(&tangents[i]);
        XMVECTOR b = XMLoadFloat3(&bitangents[i]);

        // https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
        XMVECTOR to = XMVector3Normalize(t - n * XMVector3Dot(n, t));

        XMVECTOR cross = XMVector3Cross(n, to);
        float handedness = XMVectorGetX(XMVector3Dot(cross, b)) < 0.0f ? -1.0f : 1.0f;

        XMFLOAT3 tf;
        XMStoreFloat3(&tf, to);

        v.tangent.x = tf.x;
        v.tangent.y = tf.y;
        v.tangent.z = tf.z;
        v.tangent.w = handedness;
    }
}

Model *ModelLoader::Load(const std::string &path) {
    size_t lastSlash = path.find_last_of("/\\");
    std::string baseDir = lastSlash != std::string::npos ? path.substr(0, lastSlash + 1) : "";

    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;

    bool success = tinyobj::LoadObj(&attributes, &shapes, &materials, &error, path.c_str(), baseDir.c_str(), true);

    if (!error.empty())
        LogWarn("OBJ error or warning: %s\n", error.c_str());

    if (!success) {
        LogWarn("Failed to load OBJ file '%s'\n", path.c_str());
        return nullptr;
    }

    baseDir = baseDir.substr(this->assetManager->GetAssetDirectory().size());

    std::vector<AssetHandle<Material>> modelMaterials(materials.size());

    for (int i = 0; i < materials.size(); ++i) {
        std::string materialName = path.substr(this->assetManager->GetAssetDirectory().size()) + "::" + materials[i].name;
        AssetID materialID = AssetID::FromHash(materialName);

        modelMaterials[i] = this->assetManager->GetHandle<Material>(materialID);

        if (this->assetManager->Has<Material>(materialID))
            continue;

        Material *newMaterial = new Material();

        Material *defaultMaterial = this->assetManager->GetDefault<Material>();

        newMaterial->pipelineState = defaultMaterial->pipelineState;

        if (!materials[i].diffuse_texname.empty()) {
            AssetID textureID = this->assetManager->PathToUUID(baseDir + materials[i].diffuse_texname);
            newMaterial->diffuseTexture = this->assetManager->GetHandle<Texture2D>(textureID);
        }
        else {
            newMaterial->diffuseTexture = defaultMaterial->diffuseTexture;
        }

        if (!materials[i].normal_texname.empty()) {
            AssetID textureID = this->assetManager->PathToUUID(baseDir + materials[i].normal_texname);
            newMaterial->normalTexture = this->assetManager->GetHandle<Texture2D>(textureID);
        }
        else if (!materials[i].bump_texname.empty()) {
            AssetID textureID = this->assetManager->PathToUUID(baseDir + materials[i].bump_texname);
            newMaterial->normalTexture = this->assetManager->GetHandle<Texture2D>(textureID);
        }
        else {
            newMaterial->normalTexture = defaultMaterial->normalTexture;
        }

        newMaterial->ambientColour = XMFLOAT3(materials[i].ambient);

        if (materials[i].diffuse[0] == 0.0f && materials[i].diffuse[1] == 0.0f && materials[i].diffuse[2] == 0.0f)
            newMaterial->diffuseColour = XMFLOAT3(1.0f, 1.0f, 1.0f);
        else
            newMaterial->diffuseColour = XMFLOAT3(materials[i].diffuse);

        newMaterial->specularColour = XMFLOAT3(materials[i].specular);

        if (materials[i].shininess <= 1.0f)
            newMaterial->specularExponent = defaultMaterial->specularExponent;
        else
            newMaterial->specularExponent = materials[i].shininess;

        this->assetManager->AddAsset<Material>(newMaterial, materialID);
    }

    struct Mesh_bucket {
        std::vector<Vertex> vertices;
        std::vector<UINT> indices;

        std::unordered_map<Vertex, UINT> uniqueVertices;
    };

    std::vector<Mesh_bucket> buckets(materials.size() + 1);

    for (const tinyobj::shape_t& shape : shapes) {
        int indexOffset = 0;

        for (int faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size(); ++faceIndex) {
            int vertexCount = shape.mesh.num_face_vertices[faceIndex];

            int materialID = shape.mesh.material_ids[faceIndex];
            materialID = (materialID < 0) ? materials.size() : materialID;

            for (int vertexIndex = vertexCount - 1; vertexIndex >= 0; --vertexIndex) {
                tinyobj::index_t index = shape.mesh.indices[indexOffset + vertexIndex];

                Vertex vertex{};

                vertex.position.x =  attributes.vertices[3 * index.vertex_index];
                vertex.position.y =  attributes.vertices[3 * index.vertex_index + 1];
                vertex.position.z = -attributes.vertices[3 * index.vertex_index + 2];

                if (index.normal_index >= 0) {
                    vertex.normal.x =  attributes.normals[3 * index.normal_index];
                    vertex.normal.y =  attributes.normals[3 * index.normal_index + 1];
                    vertex.normal.z = -attributes.normals[3 * index.normal_index + 2];
                }

                if (index.texcoord_index >= 0) {
                    vertex.uv.x = attributes.texcoords[2 * index.texcoord_index];
                    vertex.uv.y = 1.0f - attributes.texcoords[2 * index.texcoord_index + 1];
                }

                Mesh_bucket &bucket = buckets[materialID];

                auto iter = bucket.uniqueVertices.find(vertex);
                if (iter != bucket.uniqueVertices.end()) {
                    bucket.indices.push_back(iter->second);
                }
                else {
                    UINT newIndex = bucket.vertices.size();

                    bucket.vertices.push_back(vertex);
                    bucket.indices.push_back(newIndex);

                    bucket.uniqueVertices[vertex] = newIndex;
                }
            }

            indexOffset += vertexCount;
        }
    }

    Model *newModel = new Model();
    std::vector<Vertex> finalVertices;
    std::vector<UINT> finalIndices;

    for (int i = 0; i < buckets.size(); ++i) {
        Mesh_bucket &bucket = buckets[i];

        if (bucket.vertices.empty())
            continue;

        Model::Sub_model subModel;

        subModel.mesh.startIndex = finalIndices.size();
        subModel.mesh.indexCount = bucket.indices.size();
        subModel.mesh.baseVertex = 0;

        BoundingBox::CreateFromPoints(subModel.localBounds, bucket.vertices.size(), &bucket.vertices[0].position, sizeof(Vertex));
        BoundingBox::CreateMerged(newModel->localBounds, newModel->localBounds, subModel.localBounds);

        if (i < modelMaterials.size())
            subModel.material = modelMaterials[i];
        else
            subModel.material = this->assetManager->GetHandle<Material>(AssetID::invalid);

        int indexOffset = finalVertices.size();
        for (UINT index : bucket.indices)
            finalIndices.push_back(index + indexOffset);

        finalVertices.insert(finalVertices.end(), bucket.vertices.begin(), bucket.vertices.end());

        newModel->subModels.push_back(subModel);
    }

    this->ComputeTangents(finalVertices, finalIndices);

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * finalVertices.size();
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = finalVertices.data();

    HRESULT result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &newModel->vertexBuffer);
    if (FAILED(result)) {
        LogWarn("Failed to create vertex buffer\n");
        delete newModel;
        return nullptr;
    }

    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.ByteWidth = sizeof(UINT) * finalIndices.size();
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = finalIndices.data();

    result = device->CreateBuffer(&indexBufferDesc, &indexData, &newModel->indexBuffer);
    if (FAILED(result)) {
        LogWarn("Failed to create index buffer\n");
        delete newModel;
        return nullptr;
    }

    LogInfo("Model '%s' loaded successfully\n", path.c_str());

    return newModel;
}

Model *ModelLoader::CreateDefault() {
    LogInfo("Default Model asset created\n");

    return new Model();
}