#include "stdafx.h"
#include "Model.h"
#include "Image.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "zlib.h"

namespace Utils {

Scene::~Scene(void) {
    for (auto image : mImages) { delete image; }
    mImages.clear();
}

Scene * Model::LoadFromFile(const char *fileName) {
    Assimp::Importer aiImporter;

    // remove unused data
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS);
    // max triangles and vertices per mesh, splits above this threshold
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, INT_MAX);
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 0xfffe); // avoid the primitive restart index
    // remove points and lines
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    const aiScene *scene = aiImporter.ReadFile(fileName,
        aiProcess_CalcTangentSpace 
        | aiProcess_JoinIdenticalVertices
        | aiProcess_Triangulate
        | aiProcess_RemoveComponent
        | aiProcess_GenSmoothNormals
        //| aiProcess_SplitLargeMeshes
        | aiProcess_ValidateDataStructure
        // | aiProcess_ImproveCacheLocality // handled by optimizePostTransform()
        | aiProcess_RemoveRedundantMaterials
        | aiProcess_SortByPType
        | aiProcess_FindInvalidData
        | aiProcess_GenUVCoords
        | aiProcess_TransformUVCoords
        //| aiProcess_OptimizeMeshes
        //| aiProcess_OptimizeGraph
    );

    if (!scene) {
        Print("Loader: load model %s failed!\n", fileName);
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

    Scene *out = new Scene;
    out->mShapes.resize(scene->mNumMeshes);
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        assert(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

        Scene::Shape &shape = out->mShapes[i];
        shape.indexOffset = indexCount;
        shape.indexCount = mesh->mNumFaces * 3;

        vertexCount += mesh->mNumVertices;
        indexCount += shape.indexCount;

        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        aiString diffuseTex;
        aiString specularTex;
        aiString normalTex;
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffuseTex)) {
            shape.diffuseTex = AddImage(images, diffuseTex);
        }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), specularTex)) {
            shape.specularTex = AddImage(images, specularTex);
        }
        if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normalTex)) {
            shape.normalTex = AddImage(images, normalTex);
        }
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

    // images
    out->mImages.resize(images.size());
    char imagePath[MAX_PATH];
    for (uint32_t i = 0; i < images.size(); ++i) {
        strcpy(imagePath, resPath);
        strcat(imagePath, images[i].c_str());
        out->mImages[i] = Image::CreateFromFile(imagePath);
    }

    return out;
}

Scene * Model::LoadFromMMB(const char *fileName) {
    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "rb")) {
        Print("Loader: open file %s failed!\n", fileName);
        return nullptr;
    }

    Scene *scene = nullptr;

    Header head;
    fread(&head, sizeof(Header), 1, binFile);
    if (!strcmp(head.tag, "mmb")) {
        Bytef *compressedData = (Bytef *)malloc(head.compressedSize);
        fread(compressedData, sizeof(Bytef), head.compressedSize, binFile);
        uint8_t *data = (uint8_t *)malloc(head.dataSize);
        uLongf dataSize = head.dataSize;
        uncompress(data, &dataSize, compressedData, head.compressedSize);
        assert(dataSize == head.dataSize);

        uint32_t vertexSize = head.vertexCount * sizeof(Scene::Vertex);
        uint32_t indexSize = head.indexCount * sizeof(uint32_t);
        uint32_t shapeSize = head.shapeCount * sizeof(Scene::Shape);
        uint32_t imageSize = head.imageCount * sizeof(Image);

        scene = new Scene;
        scene->mVertices.resize(head.vertexCount);
        scene->mIndices.resize(head.indexCount);
        scene->mShapes.resize(head.shapeCount);
        scene->mImages.resize(head.imageCount);
        uint8_t *source = data;
        memcpy(scene->mVertices.data(), source, vertexSize);
        source += vertexSize;
        memcpy(scene->mIndices.data(), source, indexSize);
        source += indexSize;
        memcpy(scene->mShapes.data(), source, shapeSize);
        source += shapeSize;

        void *imageHead = malloc(sizeof(Image));
        uint8_t *heads = source;
        uint8_t *pixels = source + imageSize;
        for (uint32_t i = 0; i < head.imageCount; ++i) {
            memcpy(imageHead, heads, sizeof(Image));
            heads += sizeof(Image);
            Image *image = Image::LoadFromBinary(imageHead, pixels);
            pixels += image->GetPixelsSize();
            scene->mImages[i] = image;
        }
        free(imageHead);
        free(compressedData);
        free(data);
    } else {
        Print("Loader: unknow format, open file %s failed!\n", fileName);
    }

    fclose(binFile);

    return scene;
}

void Model::SaveToMMB(const Scene *scene, const char *fileName) {
    if (!scene) {
        Print("Loader: empty scene object!\n");
        return;
    }

    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "wb")) {
        Print("Loader: open file %s failed!\n", fileName);
        return;
    }

    // Copy data
    uint32_t vertexSize = static_cast<uint32_t>(scene->mVertices.size() * sizeof(Scene::Vertex));
    uint32_t indexSize = static_cast<uint32_t>(scene->mIndices.size() * sizeof(uint32_t));
    uint32_t shapeSize = static_cast<uint32_t>(scene->mShapes.size() * sizeof(Scene::Shape));
    uint32_t imageSize = static_cast<uint32_t>(scene->mImages.size() * sizeof(Image));
    uint32_t pixelSize = 0;
    for (auto image : scene->mImages) {
        pixelSize += image->GetPixelsSize();
    }
    uint32_t dataSize = vertexSize + indexSize + shapeSize + imageSize + pixelSize;
    uint8_t *data = (uint8_t *)malloc(dataSize);
    uint8_t *dest = data;
    memcpy(dest, scene->mVertices.data(), vertexSize);
    dest += vertexSize;
    memcpy(dest, scene->mIndices.data(), indexSize);
    dest += indexSize;
    memcpy(dest, scene->mShapes.data(), shapeSize);
    dest += shapeSize;
    for (Image *image : scene->mImages) {
        memcpy(dest, image, sizeof(Image));
        dest += sizeof(Image);
    }
    for (Image *image : scene->mImages) {
        uint32_t copySize = image->GetPixelsSize();
        memcpy(dest, image->GetPixels(), copySize);
        dest += copySize;
    }
    // Compress data
    uLong compressedSize = compressBound(dataSize);
    Bytef *compressedData = (Bytef *)malloc(compressedSize);
    compress(compressedData, &compressedSize, (const Bytef *)data, dataSize);

    Header head = {
        {'m', 'm', 'b', '\0'},
        static_cast<uint32_t>(scene->mVertices.size()),
        static_cast<uint32_t>(scene->mIndices.size()),
        static_cast<uint32_t>(scene->mShapes.size()),
        static_cast<uint32_t>(scene->mImages.size()),
        dataSize,
        compressedSize
    };

    fwrite(&head, sizeof(Header), 1, binFile);
    fwrite(compressedData, sizeof(Bytef), compressedSize, binFile);

    fclose(binFile);

    free(data);
    free(compressedData);
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
    shape.diffuseTex = ~0;
    shape.specularTex = ~0;
    shape.normalTex = ~0;

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
    shape.diffuseTex = ~0;
    shape.specularTex = ~0;
    shape.normalTex = ~0;

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

    for (size_t i = 0; i < scene->mIndices.size(); i += 3) {
        uint32_t i0 = scene->mIndices[i + 0];
        uint32_t i1 = scene->mIndices[i + 1];
        uint32_t i2 = scene->mIndices[i + 2];

        Scene::Vertex &vertex0 = scene->mVertices[i0];
        Scene::Vertex &vertex1 = scene->mVertices[i1];
        Scene::Vertex &vertex2 = scene->mVertices[i2];

        // Shortcuts for vertices
        XMFLOAT3 &v0 = vertex0.position;
        XMFLOAT3 &v1 = vertex1.position;
        XMFLOAT3 &v2 = vertex2.position;

        // Shortcuts for UVs
        XMFLOAT2 &uv0 = vertex0.texCoord;
        XMFLOAT2 &uv1 = vertex1.texCoord;
        XMFLOAT2 &uv2 = vertex2.texCoord;

        // Edges of the triangle : position delta
        XMFLOAT3 deltaPos1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
        XMFLOAT3 deltaPos2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);

        // UV delta
        XMFLOAT2 deltaUV1(v1.x - v0.x, v1.y - v0.y);
        XMFLOAT2 deltaUV2(v2.x - v0.x, v2.y - v0.y);

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        XMFLOAT3 tangent((deltaPos1.x * deltaUV2.y   - deltaPos2.x * deltaUV1.y) * r,
                         (deltaPos1.y * deltaUV2.y   - deltaPos2.y * deltaUV1.y) * r,
                         (deltaPos1.z * deltaUV2.y   - deltaPos2.z * deltaUV1.y) * r);

        XMFLOAT3 bitangent((deltaPos2.x * deltaUV1.x   - deltaPos1.x * deltaUV2.x) * r,
                           (deltaPos2.y * deltaUV1.x   - deltaPos1.y * deltaUV2.x) * r,
                           (deltaPos2.z * deltaUV1.x   - deltaPos1.z * deltaUV2.x) * r);

        // Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vboindexer.cpp
        vertex0.tangent = tangent;
        vertex1.tangent = tangent;
        vertex2.tangent = tangent;

        // Same thing for bitangents
        vertex0.bitangent = bitangent;
        vertex1.bitangent = bitangent;
        vertex2.bitangent = bitangent;
    }

    auto &shape = scene->mShapes[0];
    shape.indexOffset = 0;
    shape.indexCount = static_cast<uint32_t>(scene->mIndices.size());
    shape.diffuseTex = ~0;
    shape.specularTex = ~0;
    shape.normalTex = ~0;

    return scene;
}

}
