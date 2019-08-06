#include "stdafx.h"
#include "Model.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "zlib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Utils {

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
    std::vector<std::string> images(scene->mNumMaterials * 3);

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
    stbi_set_flip_vertically_on_load(1);
    for (uint32_t i = 0; i < images.size(); ++i) {
        strcpy(imagePath, resPath);
        strcat(imagePath, images[i].c_str());

        Scene::Image &image = out->mImages[i];

        int width, height, channels;
        stbi_uc *pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            Print("Loader: load image file failed!\n");
            continue;
        }

        image.width = width;
        image.height = height;
        image.channels = 4;
        size_t dataSize = width * height * 4;
        image.pixels = (uint8_t *)malloc(dataSize);
        memcpy(image.pixels, pixels, dataSize);

        stbi_image_free(pixels);
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
        uint32_t imageSize = head.imageCount * sizeof(Scene::Image);

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
        memcpy(scene->mImages.data(), source, imageSize);
        source += imageSize;
        for (uint32_t i = 0; i < scene->mImages.size(); ++i) {
            Scene::Image &image = scene->mImages[i];
            uint32_t copySize = image.width * image.height * image.channels;
            image.pixels = (uint8_t *)malloc(copySize);
            memcpy(image.pixels, source, copySize);
            source += copySize;
        }

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
    uint32_t imageSize = static_cast<uint32_t>(scene->mImages.size() * sizeof(Scene::Image));
    uint32_t pixelSize = 0;
    for (uint32_t i = 0; i < scene->mImages.size(); ++i) {
        const Scene::Image &image = scene->mImages[i];
        pixelSize += image.width * image.height * image.channels;
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
    memcpy(dest, scene->mImages.data(), imageSize);
    dest += imageSize;
    for (uint32_t i = 0; i < scene->mImages.size(); ++i) {
        const Scene::Image &image = scene->mImages[i];
        uint32_t copySize = image.width * image.height * image.channels;
        memcpy(dest, image.pixels, copySize);
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

}
