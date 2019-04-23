#include "pch.h"
#include "ObjLoader.h"

extern "C" 
{
    #define TINYOBJ_LOADER_C_IMPLEMENTATION
    #include "tinyobj_loader_c.h"
}

ObjLoader::ObjLoader(const char *fileName, bool isBinary)
{
    isBinary ? LoadFromBinaryFile(fileName) : LoadFromTextFile(fileName);
}

ObjLoader::~ObjLoader(void) {

}

void ObjLoader::LoadFromTextFile(const char *fileName) {
    FILE *objFile = nullptr;
    char *fileContent;
    long fileSize;

    // fopen_s return zero if success
    if (fopen_s(&objFile, fileName, "rb")) {
        Print("ObjLoader: open file %s failed!\n", fileName);
        return;
    }

    fseek(objFile, 0, SEEK_END);
    fileSize = ftell(objFile);
    
    fileContent = new char[fileSize];
    fseek(objFile, 0, SEEK_SET);
    fread(fileContent, sizeof(char), fileSize, objFile);
    fclose(objFile);

    tinyobj_attrib_t attrib;
    tinyobj_shape_t *shapes;
    tinyobj_material_t *materials;
    size_t shapeCount, materialCount;
    float defaultTexCoord[2] = { 0.0f, 0.0f };
    if (TINYOBJ_SUCCESS == tinyobj_parse_obj(&attrib, &shapes, &shapeCount, &materials, &materialCount, fileContent, fileSize, TINYOBJ_FLAG_TRIANGULATE)) {
        for (size_t i = 0; i < shapeCount; ++i) {
            Shape shape;
            shape.name = shapes[i].name;
            shape.fromIndex = mIndices.Count();
            uint32_t offset = shapes[i].face_offset;
            for (uint32_t j = 0; j < shapes[i].length; ++j) {
                for (uint32_t k = 0; k < 3; ++k) {
                    uint32_t index = (j + offset) * 3 + k;
                    assert(index < attrib.num_faces);
                    float *position = attrib.vertices + attrib.faces[index].v_idx * 3;
                    float *texCoord = attrib.num_texcoords ? attrib.texcoords + attrib.faces[index].v_idx * 2 : defaultTexCoord;
                    uint32_t next = mVertices.Count();
                    for (uint32_t n = 0; n < mVertices.Count(); ++n) {
                        const Vertex &v = mVertices.At(n);
                        if (v.position.x == position[0] && v.position.y == position[1] && v.position.z == position[2]
                            && v.texCoord.x == texCoord[0] && v.texCoord.y == texCoord[1]) {
                            next = n;
                            break;
                        }
                    }
                    if (next == mVertices.Count()) {
                        mVertices.PushBack({ {position[0], position[1], position[2]}, {texCoord[0], texCoord[1]} });
                    }
                    mIndices.PushBack(next);
                }
            }
            shape.indexCount = mIndices.Count() - shape.fromIndex;
            mShapes.PushBack(shape);
        }

        tinyobj_attrib_free(&attrib);
        if (shapes) {
            tinyobj_shapes_free(shapes, shapeCount);
        }
        if (materials) {
            tinyobj_materials_free(materials, materialCount);
        }
    } else {
        Print("ObjLoader: parse %s failed!\n", fileName);
    }

    delete [] fileContent;
}

void ObjLoader::LoadFromBinaryFile(const char *fileName) {
    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "rb")) {
        Print("ObjLoader: open file %s failed!\n", fileName);
        return;
    }

    Header head;
    fread(&head, sizeof(Header), 1, binFile);
    if (!strcmp(head.tag, "bmx")) {
        mShapes.Resize(head.shapeCount);
        for (uint32_t i = 0; i < head.shapeCount; ++i) {
            Shape &shape = mShapes.At(i);
            uint32_t nameLen;
            fread(&nameLen, sizeof(uint32_t), 1, binFile);
            char *name = new char[nameLen];
            fread(name, sizeof(char), nameLen, binFile);
            shape.name = name;
            delete [] name;

            fread(&shape.fromIndex, sizeof(uint32_t), 1, binFile);
            fread(&shape.indexCount, sizeof(uint32_t), 1, binFile);
        }

        mVertices.Resize(head.vertexCount);
        fread(mVertices.Data(), sizeof(Vertex), head.vertexCount, binFile);

        mIndices.Resize(head.indexCount);
        fread(mIndices.Data(), sizeof(uint32_t), head.indexCount, binFile);

    } else {
        Print("ObjLoader: unknow format, open file %s failed!\n", fileName);
    }

    fclose(binFile);
}

void ObjLoader::SaveToBinaryFile(const char *fileName) {
    FILE *binFile = nullptr;
    if (fopen_s(&binFile, fileName, "wb")) {
        Print("ObjLoader: open file %s failed!\n", fileName);
        return;
    }

    Header head = { 
        {'b', 'm', 'x', '\0'},
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

    fwrite(mVertices.Data(), sizeof(Vertex), head.vertexCount, binFile);

    fwrite(mIndices.Data(), sizeof(uint32_t), head.indexCount, binFile);

    fclose(binFile);
}
