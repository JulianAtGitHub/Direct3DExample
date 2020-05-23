#include "stdafx.h"
#include "Model.h"
#include "Image.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"

namespace Utils {

static const uint32_t aiProcess_NoFlag = 0;

static void PrintMaterialInfo(aiMaterial *material) {
    if (material == nullptr) {
        return;
    }

    //aiString name; if (aiReturn_SUCCESS == material->Get(AI_MATKEY_NAME, name)) { DEBUG_PRINT("Material: %s", name.C_Str()); }

    //#define PRINT_MAT_TEXTURE(type, i) { aiString tex; if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TEXTURE(type, i), tex)) { DEBUG_PRINT("%s, %s", #type, tex.C_Str()); } }

    //PRINT_MAT_TEXTURE(aiTextureType_DIFFUSE, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_SPECULAR, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_AMBIENT, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_EMISSIVE, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_HEIGHT, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_NORMALS, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_SHININESS, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_OPACITY, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_DISPLACEMENT, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_LIGHTMAP, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_REFLECTION, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_BASE_COLOR, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_NORMAL_CAMERA, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_EMISSION_COLOR, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_METALNESS, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0);
    //PRINT_MAT_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0);
}

Scene::Shape::Shape(void)
: indexOffset(0)
, indexCount(0)
, materialIndex(0)
{

}

Scene::Material::Material(void)
: normalScale(1.0f)
, normalTexture(TEX_INDEX_INVALID)
, occlusionStrength(1.0f)
, occlusionTexture(TEX_INDEX_INVALID)
, emissiveFactor(0.0f, 0.0f, 0.0f)
, emissiveTexture(TEX_INDEX_INVALID)
, baseFactor(1.0f, 1.0f, 1.0f, 1.0f)
, baseTexture(TEX_INDEX_INVALID)
, metallicFactor(1.0f)
, metallicTexture(TEX_INDEX_INVALID)
, roughnessFactor(1.0f)
, roughnessTexture(TEX_INDEX_INVALID)
, isOpacity(true) 
{

}

Scene::Scene(void) {
    XMStoreFloat4x4(&mTransform, XMMatrixIdentity());
}

Scene::~Scene(void) {
    for (auto image : mImages) { delete image; }
    mImages.clear();
}

Scene * Model::LoadFromFile(const char *fileName) {
    Assimp::Importer aiImporter;

    // max triangles and vertices per mesh, splits above this threshold
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, INT_MAX);
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 0xfffe); // avoid the primitive restart index
    // remove points and lines
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    const aiScene *scene = aiImporter.ReadFile(fileName,
                                                aiProcess_NoFlag
                                                | aiProcess_FlipUVs
                                                | aiProcess_CalcTangentSpace 
                                                | aiProcess_JoinIdenticalVertices
                                                | aiProcess_Triangulate
                                                | aiProcess_GenSmoothNormals
                                                | aiProcess_ValidateDataStructure
                                                | aiProcess_RemoveRedundantMaterials
                                                | aiProcess_SortByPType
                                                | aiProcess_FindInvalidData
                                                );

    if (!scene) {
        Print("Loader: load model %s failed with error %s \n", fileName, aiImporter.GetErrorString());
        return nullptr;
    }

    char resPath[MAX_PATH] = {'\0'};
    const char *lastSlash = strrchr(fileName, '\\');
    if (lastSlash) {
        size_t size = lastSlash - fileName + 1;
        memcpy(resPath, fileName, size);
        resPath[size] = '\0';
    }

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    Scene *out = new Scene;
    out->mShapes.reserve(scene->mNumMeshes);
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        assert(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

        Scene::Shape shape;
        shape.name = mesh->mName.C_Str();
        shape.indexOffset = indexCount;
        shape.indexCount = mesh->mNumFaces * 3;
        shape.materialIndex = mesh->mMaterialIndex;

        vertexCount += mesh->mNumVertices;
        indexCount += shape.indexCount;

        out->mShapes.push_back(shape);
    }

    out->mVertices.resize(vertexCount);
    out->mIndices.resize(indexCount);
    Scene::Vertex *vertex = out->mVertices.data();
    uint32_t *index = out->mIndices.data();
    uint32_t indexOffset = 0;
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];

        // vertex data
        for (uint32_t j = 0; j < mesh->mNumVertices; ++j) {
            // position
            assert(mesh->mVertices);
            vertex->position.x = mesh->mVertices[j].x;
            vertex->position.y = mesh->mVertices[j].y;
            vertex->position.z = mesh->mVertices[j].z;

            // texCoord
            if (mesh->mTextureCoords[0]) {
                vertex->texCoord.x = mesh->mTextureCoords[0][j].x;
                vertex->texCoord.y = mesh->mTextureCoords[0][j].y;
            } else {
                vertex->texCoord = { 0.0f, 0.0f };
            }

            // normal
            assert(mesh->mNormals);
            vertex->normal.x = mesh->mNormals[j].x;
            vertex->normal.y = mesh->mNormals[j].y;
            vertex->normal.z = mesh->mNormals[j].z;

            // tangent
            if (mesh->mTangents) {
                vertex->tangent.x = mesh->mTangents[j].x;
                vertex->tangent.y = mesh->mTangents[j].y;
                vertex->tangent.z = mesh->mTangents[j].z;
            } else {
                vertex->tangent = { 1.0f, 0.0f, 0.0f };
            }

            // bitangent
            if (mesh->mBitangents) {
                vertex->bitangent.x = mesh->mBitangents[j].x;
                vertex->bitangent.y = mesh->mBitangents[j].y;
                vertex->bitangent.z = mesh->mBitangents[j].z;
            } else {
                vertex->bitangent = { 0.0f, 1.0f, 0.0f };
            }

            vertex++;
        }

        // index data
        for (uint32_t j = 0; j < mesh->mNumFaces; ++j) {
            assert(mesh->mFaces[j].mNumIndices == 3);
            *index++ = mesh->mFaces[j].mIndices[0] + indexOffset;
            *index++ = mesh->mFaces[j].mIndices[1] + indexOffset;
            *index++ = mesh->mFaces[j].mIndices[2] + indexOffset;
        }

        indexOffset += mesh->mNumVertices;
    }

    auto AddImage = [](std::vector<std::string> &images, aiString &newImage)->uint32_t {
        uint32_t idx = static_cast<uint32_t>(images.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(images.size()); ++i) {
            if (images[i] == newImage.C_Str()) {
                idx = i;
                break;
            }
        }
        if (idx == images.size()) {
            images.push_back(newImage.C_Str());
        }
        return idx;
    };
    std::vector<std::string> images;
    images.reserve(scene->mNumMaterials * 3);

    // material
    out->mMaterials.resize(scene->mNumMaterials);
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial *material = scene->mMaterials[i];
        if (!material) {
            continue;
        }

        auto &mat = out->mMaterials[i];
        ai_real realValue;
        aiColor3D color3Value;
        aiColor4D color4Value;
        aiString stringValue;

        // material load code check with function ImportMaterial in file assimp/code/glTF2/glTF2Importer.cpp
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), realValue)) { mat.normalScale = realValue; }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), stringValue)) { mat.normalTexture = AddImage(images, stringValue); }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0), realValue)) { mat.occlusionStrength = realValue; }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, 0), stringValue)) { mat.occlusionTexture = AddImage(images, stringValue); }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), stringValue)) { mat.emissiveTexture = AddImage(images, stringValue); mat.emissiveFactor = XMFLOAT3(1.0f, 1.0f, 1.0f); }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, color3Value)) { mat.emissiveFactor = XMFLOAT3(color3Value.r, color3Value.g, color3Value.b); }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, color4Value)) { mat.baseFactor = XMFLOAT4(color4Value.r, color4Value.g, color4Value.b, color4Value.a); }
        if (aiReturn_SUCCESS == material->Get(_AI_MATKEY_TEXTURE_BASE, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, stringValue)) { mat.baseTexture = AddImage(images, stringValue); }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, realValue)) { mat.metallicFactor = realValue; }
        if (aiReturn_SUCCESS == material->Get(_AI_MATKEY_TEXTURE_BASE, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, stringValue)) { mat.metallicTexture = AddImage(images, stringValue); }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, realValue)) { mat.roughnessFactor = realValue; }
        if (aiReturn_SUCCESS == material->Get(_AI_MATKEY_TEXTURE_BASE, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, stringValue)) { mat.roughnessTexture = AddImage(images, stringValue); }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_GLTF_ALPHAMODE, stringValue)) { mat.isOpacity = (stringValue == aiString("OPAQUE")); }
    }

    // images
    out->mImages.resize(images.size());
    char imagePath[MAX_PATH];
    for (uint32_t i = 0; i < images.size(); ++i) {
        strcpy(imagePath, resPath);
        strcat(imagePath, images[i].c_str());
        out->mImages[i] = Image::CreateFromFile(imagePath);
    }

    // transform
    if (scene->mRootNode) {
        out->mTransform = *((XMFLOAT4X4 *)&(scene->mRootNode->mTransformation));
    }

    return out;
}

Scene * Model::CreateUnitQuad(void) {
    Scene *scene = new Scene;
    scene->mVertices.reserve(4);
    scene->mIndices.reserve(6);
    scene->mShapes.resize(1);

    scene->mVertices.push_back({ {-1.0f, 0.0f,-1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f, 0.0f,-1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    
    scene->mIndices.push_back(0);
    scene->mIndices.push_back(3);
    scene->mIndices.push_back(2);
    scene->mIndices.push_back(0);
    scene->mIndices.push_back(2);
    scene->mIndices.push_back(1);

    auto &shape = scene->mShapes[0];
    shape.indexOffset = 0;
    shape.indexCount = 6;

    return scene;
}

Scene * Model::CreateUnitCube(void) {
    Scene *scene = new Scene;
    scene->mVertices.reserve(24);
    scene->mIndices.reserve(36);
    scene->mShapes.resize(1);

    // Top
    scene->mVertices.push_back({ {-1.0f, 1.0f,-1.0f}, {0.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f, 1.0f,-1.0f}, {1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    // Front
    scene->mVertices.push_back({ {-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f,-1.0f, 1.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f,-1.0f, 1.0f}, {0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    // Back
    scene->mVertices.push_back({ {-1.0f, 1.0f,-1.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f,-1.0f}, { 0.0f,-1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f, 1.0f,-1.0f}, {1.0f, 1.0f}, { 0.0f, 0.0f,-1.0f}, { 0.0f,-1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f,-1.0f,-1.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f,-1.0f}, { 0.0f,-1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f,-1.0f,-1.0f}, {0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f}, { 0.0f,-1.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    // Left
    scene->mVertices.push_back({ {-1.0f, 1.0f,-1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f,-1.0f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f,-1.0f,-1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f}, {0.0f, 0.0f, 0.0f} });
    // Right
    scene->mVertices.push_back({ { 1.0f, 1.0f,-1.0f}, {0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f,-1.0f, 1.0f}, {1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f,-1.0f,-1.0f}, {0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} });
    // Bottom
    scene->mVertices.push_back({ {-1.0f,-1.0f,-1.0f}, {0.0f, 1.0f}, { 0.0f,-1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f,-1.0f,-1.0f}, {1.0f, 1.0f}, { 0.0f,-1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ { 1.0f,-1.0f, 1.0f}, {1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    scene->mVertices.push_back({ {-1.0f,-1.0f, 1.0f}, {0.0f, 0.0f}, { 0.0f,-1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    // Top
    scene->mIndices.push_back(0);
    scene->mIndices.push_back(3);
    scene->mIndices.push_back(2);
    scene->mIndices.push_back(0);
    scene->mIndices.push_back(2);
    scene->mIndices.push_back(1);
    // Front
    scene->mIndices.push_back(4);
    scene->mIndices.push_back(7);
    scene->mIndices.push_back(6);
    scene->mIndices.push_back(4);
    scene->mIndices.push_back(6);
    scene->mIndices.push_back(5);
    // Back
    scene->mIndices.push_back(9);
    scene->mIndices.push_back(10);
    scene->mIndices.push_back(11);
    scene->mIndices.push_back(9);
    scene->mIndices.push_back(11);
    scene->mIndices.push_back(8);
    // Left
    scene->mIndices.push_back(12);
    scene->mIndices.push_back(15);
    scene->mIndices.push_back(14);
    scene->mIndices.push_back(12);
    scene->mIndices.push_back(14);
    scene->mIndices.push_back(13);
    // Right
    scene->mIndices.push_back(17);
    scene->mIndices.push_back(18);
    scene->mIndices.push_back(19);
    scene->mIndices.push_back(17);
    scene->mIndices.push_back(19);
    scene->mIndices.push_back(16);
    // Bottom
    scene->mIndices.push_back(21);
    scene->mIndices.push_back(22);
    scene->mIndices.push_back(23);
    scene->mIndices.push_back(21);
    scene->mIndices.push_back(23);
    scene->mIndices.push_back(20);

    auto &shape = scene->mShapes[0];
    shape.indexOffset = 0;
    shape.indexCount = 36;

    return scene;
}

Scene * Model::CreateUnitSphere(void) {
    Scene *scene = new Scene;
    scene->mShapes.resize(1);

    constexpr uint32_t sectorCount = 64;
    constexpr uint32_t stackCount = 32;

    float x, y, z, xy;  // vertex position
    float s, t;         // vertex texCoord

    float sectorStep = XM_2PI / sectorCount;
    float stackStep = XM_PI / stackCount;
    float sectorAngle, stackAngle;

    for(uint32_t i = 0; i <= stackCount; ++i) {
        stackAngle = XM_PIDIV2 - i * stackStep; // starting from pi/2 to -pi/2
        xy = cosf(stackAngle);                  // r * cos(u)
        z = sinf(stackAngle);                   // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(uint32_t j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)

            // normalized vertex normal (x, y, z)

            // vertex tex coord (s, t) range between [0, 1]
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            scene->mVertices.push_back({ {x, y, z}, {s, t}, {x, y, z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
        }
    }

    // generate CCW index list of sphere triangles
    uint32_t k1, k2;
    for(uint32_t i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if(i != 0) {
                scene->mIndices.push_back(k1);
                scene->mIndices.push_back(k2);
                scene->mIndices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if(i != (stackCount-1)) {
                scene->mIndices.push_back(k1 + 1);
                scene->mIndices.push_back(k2);
                scene->mIndices.push_back(k2 + 1);
            }
        }
    }

    auto &shape = scene->mShapes[0];
    shape.indexOffset = 0;
    shape.indexCount = static_cast<uint32_t>(scene->mIndices.size());

    return scene;
}

}
