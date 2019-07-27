#include "pch.h"
#include "Model.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "zlib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Utils {

Scene * Model::LoadFromTextFile(const char *fileName) {
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

    Scene *out = new Scene;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    CList<CString> images(scene->mNumTextures);
    out->mShapes.Resize(scene->mNumMeshes);
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        assert(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

        Scene::Shape &shape = out->mShapes.At(i);
        shape.name = mesh->mName.C_Str();
        shape.fromIndex = indexCount;
        shape.indexCount = mesh->mNumFaces * 3;

        vertexCount += mesh->mNumVertices;
        indexCount += shape.indexCount;

        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        aiString imagePath;
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 
            && aiReturn_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &imagePath)) {
            uint32_t findIdx = images.Count();
            for (uint32_t i = 0; i < images.Count(); ++i) {
                if (images.At(i) == imagePath.C_Str()) {
                    findIdx = i;
                    break;
                }
            }
            if (findIdx == images.Count()) {
                images.PushBack(imagePath.C_Str());
            }
            shape.imageIndex = findIdx;
        } else {
            shape.imageIndex = -1;
        }
    }

    out->mVertices.Resize(vertexCount);
    out->mIndices.Resize(indexCount);
    Scene::Vertex *vertex = out->mVertices.Data();
    uint32_t *index = out->mIndices.Data();
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
    out->mImages.Resize(images.Count());
    char imagePath[MAX_PATH];
    stbi_set_flip_vertically_on_load(1);
    for (uint32_t i = 0; i < images.Count(); ++i) {
        strcpy(imagePath, resPath);
        strcat(imagePath, images.At(i).Get());

        Scene::Image &image = out->mImages.At(i);

        int width, height, channels;
        stbi_uc *pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            Print("Loader: load image file failed!\n");
            continue;
        }

        image.name = images.At(i);
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

Scene * Model::LoadFromBinaryFile(const char *fileName) {
    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "rb")) {
        Print("Loader: open file %s failed!\n", fileName);
        return nullptr;
    }

    Scene *out = nullptr;

    Header head;
    fread(&head, sizeof(Header), 1, binFile);
    if (!strcmp(head.tag, "bsx")) {
        char resPath[MAX_PATH] = {'\0'};
        const char *lastSlash = strrchr(fileName, '\\');
        if (lastSlash) {
            size_t size = lastSlash - fileName + 1;
            memcpy(resPath, fileName, size);
            resPath[size] = '\0';
        }

        char name[MAX_PATH];
        uint32_t nameLen;
        char imagePath[MAX_PATH];

        out = new Scene;

        // read shapes
        out->mShapes.Resize(head.shapeCount);
        for (uint32_t i = 0; i < head.shapeCount; ++i) {
            Scene::Shape &shape = out->mShapes.At(i);
            fread(&nameLen, sizeof(uint32_t), 1, binFile);
            fread(name, sizeof(char), nameLen, binFile);
            shape.name = name;

            fread(&shape.fromIndex, sizeof(uint32_t), 1, binFile);
            fread(&shape.indexCount, sizeof(uint32_t), 1, binFile);
            fread(&shape.imageIndex, sizeof(uint32_t), 1, binFile);
        }

        // read images
        stbi_set_flip_vertically_on_load(1);
        out->mImages.Resize(head.imageCount);
        for (uint32_t i = 0; i < head.imageCount; ++i) {
            Scene::Image &image = out->mImages.At(i);
            fread(&nameLen, sizeof(uint32_t), 1, binFile);
            fread(name, sizeof(char), nameLen, binFile);
            image.name = name;

            // load image
            strcpy(imagePath, resPath);
            strcat(imagePath, name);

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

        // read vertices
        out->mVertices.Resize(head.vertexCount);
        uLong vertexBufferSize = sizeof(Scene::Vertex) * head.vertexCount;
        uLong vertSize = 0;
        fread(&vertSize, sizeof(uLong), 1, binFile);
        Bytef *vertices = (Bytef *)malloc(vertSize);
        fread(vertices, sizeof(Bytef), vertSize, binFile);
        uncompress((Bytef *)out->mVertices.Data(), &vertexBufferSize, vertices, vertSize);
        assert(vertexBufferSize == sizeof(Scene::Vertex) * head.vertexCount);

        // read indices
        out->mIndices.Resize(head.indexCount);
        uLong indexBufferSize = sizeof(uint32_t)* head.indexCount;
        uLong idxSize = 0;
        fread(&idxSize, sizeof(uLong), 1, binFile);
        Bytef *indices = (Bytef *)malloc(idxSize);
        fread(indices, sizeof(Bytef), idxSize, binFile);
        uncompress((Bytef *)out->mIndices.Data(), &indexBufferSize, indices, idxSize);
        assert(indexBufferSize == sizeof(uint32_t)* head.indexCount);

    } else {
        Print("Loader: unknow format, open file %s failed!\n", fileName);
    }

    fclose(binFile);

    return out;
}

void Model::SaveToBinaryFile(const Scene *scene, const char *fileName) {
    if (!scene) {
        Print("Loader: empty scene object!\n");
        return;
    }

    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "wb")) {
        Print("Loader: open file %s failed!\n", fileName);
        return;
    }

    Header head = {
        {'b', 's', 'x', '\0'},
        scene->mShapes.Count(),
        scene->mVertices.Count(),
        scene->mIndices.Count(),
        scene->mImages.Count()
    };

    fwrite(&head, sizeof(Header), 1, binFile);

    // shapes
    for (uint32_t i = 0; i < scene->mShapes.Count(); ++i) {
        const Scene::Shape &shape = scene->mShapes.At(i);
        uint32_t nameLen = (uint32_t)shape.name.Length() + 1;
        fwrite(&nameLen, sizeof(uint32_t), 1, binFile);
        fwrite(shape.name.Get(), sizeof(char), nameLen, binFile);
        fwrite(&shape.fromIndex, sizeof(uint32_t), 1, binFile);
        fwrite(&shape.indexCount, sizeof(uint32_t), 1, binFile);
        fwrite(&shape.imageIndex, sizeof(uint32_t), 1, binFile);
    }

    // images
    for (uint32_t i = 0; i < scene->mImages.Count(); ++i) {
        const CString &name = scene->mImages.At(i).name;
        uint32_t nameLen = (uint32_t)name.Length() + 1;
        fwrite(&nameLen, sizeof(uint32_t), 1, binFile);
        fwrite(name.Get(), sizeof(char), nameLen, binFile);
    }

    // write vertices
    uLong vertexBufferSize = sizeof(Scene::Vertex) * head.vertexCount;
    uLong vertSize = compressBound(vertexBufferSize);
    Bytef *vertices = (Bytef *)malloc(vertSize);
    compress(vertices, &vertSize, (const Bytef *)scene->mVertices.Data(), vertexBufferSize);
    fwrite(&vertSize, sizeof(uLong), 1, binFile);
    fwrite(vertices, sizeof(Bytef), vertSize, binFile);

    // write indices
    uLong indexBufferSize = sizeof(uint32_t)* head.indexCount;
    uLong idxSize = compressBound(indexBufferSize);
    Bytef *indices = (Bytef *)malloc(idxSize);
    compress(indices, &idxSize, (const Bytef *)scene->mIndices.Data(), indexBufferSize);
    fwrite(&idxSize, sizeof(uLong), 1, binFile);
    fwrite(indices, sizeof(Bytef), idxSize, binFile);

    fclose(binFile);

    free(vertices);
    free(indices);
}

}
