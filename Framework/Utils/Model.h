#pragma once

namespace Utils {

class Image;

class Scene {
public:
    constexpr static uint32_t TEX_INDEX_INVALID = 0xFFFFFFFF;

    struct Shape {
        Shape(void)
        : indexOffset(0)
        , indexCount(0)
        , ambientColor(0.0f, 0.0f, 0.0f)
        , diffuseColor(0.0f, 0.0f, 0.0f)
        , specularColor(0.0f, 0.0f, 0.0f)
        , emissiveColor(0.0f, 0.0f, 0.0f)
        , ambientTex(TEX_INDEX_INVALID)
        , diffuseTex(TEX_INDEX_INVALID)
        , specularTex(TEX_INDEX_INVALID)
        , normalTex(TEX_INDEX_INVALID)
        , isOpacity(true) 
        {

        }

        uint32_t indexOffset;
        uint32_t indexCount;
        XMFLOAT3 ambientColor;
        XMFLOAT3 diffuseColor;
        XMFLOAT3 specularColor;
        XMFLOAT3 emissiveColor;
        uint32_t ambientTex;    // metallic
        uint32_t diffuseTex;    // albedo
        uint32_t specularTex;   // roughness
        uint32_t normalTex;     // normal
        bool     isOpacity;
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
