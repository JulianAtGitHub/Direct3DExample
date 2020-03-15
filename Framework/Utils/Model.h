#pragma once

namespace Utils {

class Image;

class Scene {
public:
    constexpr static uint32_t TEX_INDEX_INVALID = 0xFFFFFFFF;

    struct Shape {
        Shape(void);

        std::string name;
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t materialIndex;
    };

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT3 bitangent;
    };

    struct Material {
        Material(void);

        // basic
        float    normalScale;
        uint32_t normalTexture;
        float    occlusionStrength; // ambient occlusion
        uint32_t occlusionTexture;  // red channel of texture
        XMFLOAT3 emissiveFactor;
        uint32_t emissiveTexture;
        // pbr
        XMFLOAT4 baseFactor;        // AKA albdo
        uint32_t baseTexture;
        float    metallicFactor;
        uint32_t metallicTexture;   // blue channel of texture
        float    roughnessFactor;
        uint32_t roughnessTexture;  // green channel of texture
        // others
        bool     isOpacity;
    };

    Scene(void);
    ~Scene(void);

    std::vector<Vertex>     mVertices;
    std::vector<uint32_t>   mIndices;
    std::vector<Shape>      mShapes;
    std::vector<Material>   mMaterials;
    std::vector<Image *>    mImages;
    XMFLOAT4X4              mTransform;
};

class Model {
public:
    static Scene * LoadFromFile(const char *fileName);

    static Scene * CreateUnitQuad(void);
    static Scene * CreateUnitCube(void);
    static Scene * CreateUnitSphere(void);
};

}
