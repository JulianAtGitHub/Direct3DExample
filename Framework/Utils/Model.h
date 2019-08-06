#pragma once

namespace Utils {

class Scene {
public:
    struct Shape {
        Shape(void): indexOffset(0), indexCount(0), diffuseTex(~0), specularTex(~0), normalTex(~0) { }
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t diffuseTex;
        uint32_t specularTex;
        uint32_t normalTex;
    };

    struct Image {
        Image(void): width(0), height(0), channels(0), pixels(nullptr) { }
        ~Image(void) { if (pixels) { free(pixels); } }
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        uint8_t *pixels;
    };

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT3 bitangent;
    };

    std::vector<Vertex>     mVertices;
    std::vector<uint32_t>   mIndices;
    std::vector<Shape>      mShapes;
    std::vector<Image>      mImages;
};

class Model {
public:


    static Scene * LoadFromFile(const char *fileName);
    static Scene * LoadFromMMB(const char *fileName);
    static void SaveToMMB(const Scene *scene, const char *fileName);

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
