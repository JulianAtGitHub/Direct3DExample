#pragma once

namespace Utils {

class Image;

class Scene {
public:
    struct Shape {
        Shape(void): indexOffset(0), indexCount(0), ambientTex(~0), diffuseTex(~0), specularTex(~0), normalTex(~0) { }
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t ambientTex;    // metallic
        uint32_t diffuseTex;    // albedo
        uint32_t specularTex;   // roughness
        uint32_t normalTex;     // normal
    };

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT3 bitangent;
    };

    ~Scene(void);

    std::vector<Vertex>     mVertices;
    std::vector<uint32_t>   mIndices;
    std::vector<Shape>      mShapes;
    std::vector<Image *>    mImages;
};

class Model {
public:
    static Scene * LoadFromFile(const char *fileName);
    static Scene * LoadFromMMB(const char *fileName);
    static void SaveToMMB(const Scene *scene, const char *fileName);

    static Scene * CreateUnitQuad(void);
    static Scene * CreateUnitCube(void);
    static Scene * CreateUnitSphere(void);

private:
    struct Header {
        char     tag[4];
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t shapeCount;
        uint32_t imageCount;
        uint32_t dataSize;
        uint32_t compressedSize;
    };
};

}
