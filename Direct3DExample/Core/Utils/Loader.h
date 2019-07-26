#pragma once

namespace Model {

class Scene;

class Loader {
public:
    static Scene * LoadFromTextFile(const char *fileName);
    static Scene * LoadFromBinaryFile(const char *fileName);
    static void SaveToBinaryFile(const Scene *scene, const char *fileName);

private:
    struct Header {
        char     tag[4];
        uint32_t shapeCount;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t imageCount;
    };
};

}
