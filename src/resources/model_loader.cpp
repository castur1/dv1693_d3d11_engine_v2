#include "model_loader.hpp"
#include "core/logging.hpp"
#include "resources/asset_manager.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

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

        if (materials[i].diffuse_texname.empty()) {
            newMaterial->diffuseTexture = defaultMaterial->diffuseTexture;
        }
        else {
            baseDir = baseDir.substr(this->assetManager->GetAssetDirectory().size());
            AssetID textureID = this->assetManager->PathToUUID(baseDir + materials[i].diffuse_texname);
            newMaterial->diffuseTexture = this->assetManager->GetHandle<Texture2D>(textureID);
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

        std::map<Vertex, UINT> uniqueVertices;
    };

    std::vector<Mesh_bucket> buckets(materials.size() + 1); // +1 for default material

    for (const tinyobj::shape_t& shape : shapes) {
        int indexOffset = 0;

        for (int faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size(); ++faceIndex) {
            int vertexCount = shape.mesh.num_face_vertices[faceIndex];

            int materialID = shape.mesh.material_ids[faceIndex];
            materialID = (materialID < 0) ? materials.size() : materialID; // Default material

            for (int vertexIndex = vertexCount - 1; vertexIndex >= 0; --vertexIndex) {
                tinyobj::index_t index = shape.mesh.indices[indexOffset + vertexIndex];

                Vertex vertex{};

                vertex.position[0] =  attributes.vertices[3 * index.vertex_index];
                vertex.position[1] =  attributes.vertices[3 * index.vertex_index + 1];
                vertex.position[2] = -attributes.vertices[3 * index.vertex_index + 2]; //

                if (index.normal_index >= 0) {
                    vertex.normal[0] =  attributes.normals[3 * index.normal_index];
                    vertex.normal[1] =  attributes.normals[3 * index.normal_index + 1];
                    vertex.normal[2] = -attributes.normals[3 * index.normal_index + 2]; //
                }

                if (index.texcoord_index >= 0) {
                    vertex.uv[0] = attributes.texcoords[2 * index.texcoord_index];
                    vertex.uv[1] = 1.0f - attributes.texcoords[2 * index.texcoord_index + 1]; //
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
        if (buckets[i].vertices.empty())
            continue;

        Model::Sub_model subModel;

        subModel.mesh.startIndex = finalIndices.size();
        subModel.mesh.indexCount = buckets[i].indices.size();
        subModel.mesh.baseVertex = 0;

        if (i < modelMaterials.size())
            subModel.material = modelMaterials[i];
        else
            subModel.material = this->assetManager->GetHandle<Material>(AssetID::invalid);

        int indexOffset = finalVertices.size();
        for (UINT index : buckets[i].indices)
            finalIndices.push_back(index + indexOffset);

        finalVertices.insert(finalVertices.end(), buckets[i].vertices.begin(), buckets[i].vertices.end());

        newModel->subModels.push_back(subModel);
    }

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