#include "pch.h"
#include "Loader.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "zlib.h"

namespace Model {

Loader::Loader(const char *fileName, bool isBinary)
{
    isBinary ? LoadFromBinaryFile(fileName) : LoadFromTextFile(fileName);
}

Loader::~Loader(void) {

}

void Loader::LoadFromTextFile(const char *fileName) {
    Assimp::Importer aiImporter;

    // remove unused data
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS);
    // max triangles and vertices per mesh, splits above this threshold
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, INT_MAX);
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 0xfffe); // avoid the primitive restart index
    // remove points and lines
    aiImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    const aiScene *scene = aiImporter.ReadFile(fileName,
                                                aiProcess_CalcTangentSpace |
                                                aiProcess_JoinIdenticalVertices |
                                                aiProcess_Triangulate |
                                                aiProcess_RemoveComponent |
                                                aiProcess_GenSmoothNormals |
                                                aiProcess_SplitLargeMeshes |
                                                aiProcess_ValidateDataStructure |
                                                //aiProcess_ImproveCacheLocality | // handled by optimizePostTransform()
                                                aiProcess_RemoveRedundantMaterials |
                                                aiProcess_SortByPType |
                                                aiProcess_FindInvalidData |
                                                aiProcess_GenUVCoords |
                                                aiProcess_TransformUVCoords |
                                                aiProcess_OptimizeMeshes |
                                                aiProcess_OptimizeGraph);

    if (!scene) {
        Print("ObjLoader: load model %s failed!\n", fileName);
        return;
    }

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    mShapes.Resize(scene->mNumMeshes);
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        assert(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

        Shape &shape = mShapes.At(i);
        shape.name = mesh->mName.C_Str();
        shape.fromIndex = indexCount;
        shape.indexCount = mesh->mNumFaces * 3;

        vertexCount += mesh->mNumVertices;
        indexCount += shape.indexCount;
    }

    mVertices.Resize(vertexCount);
    mIndices.Resize(indexCount);
    Vertex *vertex = mVertices.Data();
    uint32_t *index = mIndices.Data();
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

            *index++ = mesh->mFaces[j].mIndices[0];
            *index++ = mesh->mFaces[j].mIndices[1];
            *index++ = mesh->mFaces[j].mIndices[2];
        }
    }
}

void Loader::LoadFromBinaryFile(const char *fileName) {
    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "rb")) {
        Print("ObjLoader: open file %s failed!\n", fileName);
        return;
    }

    Header head;
    fread(&head, sizeof(Header), 1, binFile);
    if (!strcmp(head.tag, "bsx")) {
        mShapes.Resize(head.shapeCount);
        char name[256];
        uint32_t nameLen;
        for (uint32_t i = 0; i < head.shapeCount; ++i) {
            Shape &shape = mShapes.At(i);
            fread(&nameLen, sizeof(uint32_t), 1, binFile);
            fread(name, sizeof(char), nameLen, binFile);
            shape.name = name;

            fread(&shape.fromIndex, sizeof(uint32_t), 1, binFile);
            fread(&shape.indexCount, sizeof(uint32_t), 1, binFile);
        }

        // read vertices
        mVertices.Resize(head.vertexCount);
        uLong vertexBufferSize = sizeof(Vertex) * head.vertexCount;
        uLong vertSize = 0;
        fread(&vertSize, sizeof(uLong), 1, binFile);
        Bytef *vertices = (Bytef *)malloc(vertSize);
        fread(vertices, sizeof(Bytef), vertSize, binFile);
        uncompress((Bytef *)mVertices.Data(), &vertexBufferSize, vertices, vertSize);
        assert(vertexBufferSize == sizeof(Vertex) * head.vertexCount);

        // read indices
        mIndices.Resize(head.indexCount);
        uLong indexBufferSize = sizeof(uint32_t)* head.indexCount;
        uLong idxSize = 0;
        fread(&idxSize, sizeof(uLong), 1, binFile);
        Bytef *indices = (Bytef *)malloc(idxSize);
        fread(indices, sizeof(Bytef), idxSize, binFile);
        uncompress((Bytef *)mIndices.Data(), &indexBufferSize, indices, idxSize);
        assert(indexBufferSize == sizeof(uint32_t)* head.indexCount);

    } else {
        Print("ObjLoader: unknow format, open file %s failed!\n", fileName);
    }

    fclose(binFile);
}

void Loader::SaveToBinaryFile(const char *fileName) {
    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "wb")) {
        Print("ObjLoader: open file %s failed!\n", fileName);
        return;
    }

    Header head = {
        {'b', 's', 'x', '\0'},
        mVertices.Count(),
        mIndices.Count(),
        mShapes.Count()
    };

    fwrite(&head, sizeof(Header), 1, binFile);

    for (uint32_t i = 0; i < mShapes.Count(); ++i) {
        const Shape &shape = mShapes.At(i);
        uint32_t nameLen = (uint32_t)shape.name.Length() + 1;
        fwrite(&nameLen, sizeof(uint32_t), 1, binFile);
        fwrite(shape.name.Get(), sizeof(char), nameLen, binFile);
        fwrite(&shape.fromIndex, sizeof(uint32_t), 1, binFile);
        fwrite(&shape.indexCount, sizeof(uint32_t), 1, binFile);
    }

    // write vertices
    uLong vertexBufferSize = sizeof(Vertex) * head.vertexCount;
    uLong vertSize = compressBound(vertexBufferSize);
    Bytef *vertices = (Bytef *)malloc(vertSize);
    compress(vertices, &vertSize, (const Bytef *)mVertices.Data(), vertexBufferSize);
    fwrite(&vertSize, sizeof(uLong), 1, binFile);
    fwrite(vertices, sizeof(Bytef), vertSize, binFile);

    // write indices
    uLong indexBufferSize = sizeof(uint32_t)* head.indexCount;
    uLong idxSize = compressBound(indexBufferSize);
    Bytef *indices = (Bytef *)malloc(idxSize);
    compress(indices, &idxSize, (const Bytef *)mIndices.Data(), indexBufferSize);
    fwrite(&idxSize, sizeof(uLong), 1, binFile);
    fwrite(indices, sizeof(Bytef), idxSize, binFile);

    fclose(binFile);

    free(vertices);
    free(indices);
}

}
