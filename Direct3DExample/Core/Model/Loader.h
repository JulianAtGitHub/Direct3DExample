#pragma once

namespace Model {

class Loader {
public:
    Loader(const char *fileName, bool isBinary = true);
    ~Loader(void);

    inline void SaveToFile(const char *fileName) { SaveToBinaryFile(fileName); }

private:
    void LoadFromTextFile(const char *fileName);
    void LoadFromBinaryFile(const char *fileName);
    void SaveToBinaryFile(const char *fileName);

    struct Header {
        char     tag[4];
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t shapeCount;
    };

    struct Shape {
        CString  name;
        uint32_t fromIndex;
        uint32_t indexCount;
    };

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT3 bitangent;
    };

    CList<Vertex>   mVertices;
    CList<uint32_t> mIndices;
    CList<Shape>    mShapes;
};

}
