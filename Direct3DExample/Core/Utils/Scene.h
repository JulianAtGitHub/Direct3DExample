#pragma once

namespace Model {

class Scene {
public:
    struct Shape {
        Shape(void): fromIndex(0), indexCount(0), imageIndex(0) { }
        CString  name;
        uint32_t fromIndex;
        uint32_t indexCount;
        uint32_t imageIndex;
    };

    struct Image {
        Image(void): width(0), height(0), channels(0), pixels(nullptr) { }
        ~Image(void) { if (pixels) { free(pixels); } }
        CString  name;
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

    CList<Vertex>   mVertices;
    CList<uint32_t> mIndices;
    CList<Image>    mImages;
    CList<Shape>    mShapes;
};

}