
#include "pch.h"
#include "Ray.h"
#include "Sphere.h"
#include "HitableList.h"
#include "Camera.h"
#include "Lambertian.h"
#include "Metal.h"
#include "Dielectric.h"

static constexpr int nx = 200;
static constexpr int ny = 100;
static constexpr int ns = 100;
static constexpr int maxDepth = 50;

XMVECTOR CalculateColor(const Ray& ray, Hitable *world, int depth) {
    static XMVECTOR white = {1.0f, 1.0f, 1.0f, 0.0f};
    static XMVECTOR blue = {0.5f, 0.7f, 1.0f, 0.0f};
    Hitable::Record record;
    if (world->Hit(ray, 0.001f, 1e+38f, record)) {
        XMVECTOR attenuation;
        Ray scatter;
        if (depth < maxDepth && record.mat->Scatter(ray, record, attenuation, scatter)) {
            return attenuation * CalculateColor(scatter, world, depth + 1);
        } else {
            return { 0.0f, 0.0f, 0.0f };
        }
    } else {
        XMVECTOR direction = XMVector3Normalize(ray.Direction());
        float t = (XMVectorGetY(direction) + 1.0f) * 0.5f;
        return white * (1.0f - t) + blue * t;
    }
}

int main(int argc, char *argv[]) {
    std::vector<Material *> materials {
        new Lambertian({0.8f, 0.3f, 0.3f}),
        new Lambertian({0.8f, 0.8f, 0.0f}),
        new Metal({0.8f, 0.6f, 0.2f}, 0.2f),
        new Dielectric(1.5f)
    };
    std::vector<Hitable *> hitables {
        new Sphere({0.0f, 0.0f, -1.0f}, 0.5f, materials[0]),
        new Sphere({0.0f, -100.5f, -1.0f}, 100.0f, materials[1]),
        new Sphere({1.0f, 0.0f, -1.0f}, 0.5f, materials[2]),
        new Sphere({-1.0f, 0.0f, -1.0f}, 0.5f, materials[3])
    };

    HitableList world(hitables.data(), static_cast<int>(hitables.size()));
    Camera camera;

    std::stringstream ss;
    ss << "P3\n" << nx << " " << ny << "\n255\n";
    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            XMVECTOR col = {0.0f, 0.0f, 0.0f};
            for (int s = 0; s < ns; ++s) {
                float u = (i + RandomUnit() - 0.5f) / float(nx);
                float v = (j + RandomUnit() - 0.5f) / float(ny);
                col += CalculateColor(camera.GenRay(u, v), &world, 0);
            }
            col /= float(ns);
            // gamma correct
            col = XMVectorSqrt(col);
            int ir = int(255.0f * XMVectorGetX(col));
            int ig = int(255.0f * XMVectorGetY(col));
            int ib = int(255.0f * XMVectorGetZ(col));
            ss << ir << " " << ig << " " << ib << "\n";
        }
    }

    for (auto material : materials) {
        delete material;
    }
    materials.clear();

    for (auto hitable : hitables) {
        delete hitable;
    }
    hitables.clear();

    // write to file
    std::string file("output.ppm");
    std::ofstream ofs;
    ofs.open(file);
    ofs << ss.str();
    ofs.close();

    return 0;
}
