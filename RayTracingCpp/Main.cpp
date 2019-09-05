
#include "pch.h"
#include "Ray.h"
#include "Sphere.h"
#include "HitableList.h"
#include "Camera.h"
#include "Lambertian.h"
#include "Metal.h"
#include "Dielectric.h"

static constexpr int nx = 600;
static constexpr int ny = 400;
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

void RandomScene(std::vector<Hitable *> &hitables, std::vector<Material *> &materials) {
    Material *mat;
    Hitable *obj;

    {
        mat = new Lambertian({ 0.5f, 0.5f, 0.5f });
        obj = new Sphere({ 0.0f, -1000.0f, 0.0f }, 1000.0f, mat);
        hitables.push_back(obj);
        materials.push_back(mat);
    }

    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            float chooseMat = RandomUnit();
            XMVECTOR center = { float(a) + 0.9f * RandomUnit(), 0.2f, float(b) + 0.9f * RandomUnit() };
            XMVECTOR x = { 4.0f, 0.2f, 0.0f };
            if (XMVectorGetX(XMVector3Length(center - x)) > 0.9f) {
                if (chooseMat < 0.8f) { // lambertian
                    mat = new Lambertian({ RandomUnit() * RandomUnit(), RandomUnit() * RandomUnit(), RandomUnit() * RandomUnit() });
                } else if (chooseMat < 0.95f) { // metal
                    mat = new Metal({ 0.5f * (1.0f + RandomUnit()), 0.5f * (1.0f + RandomUnit()), 0.5f * (1.0f + RandomUnit()) }, 0.5f * RandomUnit());
                } else { // glass
                    mat = new Dielectric(1.5f);
                }
                obj = new Sphere(center, 0.2f, mat);
                hitables.push_back(obj);
                materials.push_back(mat);
            }
        }
    }

    {
        mat = new Dielectric(1.5f);
        obj = new Sphere({ 0.0f, 1.0f, 0.0f }, 1.0f, mat);
        hitables.push_back(obj);
        materials.push_back(mat);
    }

    {
        mat = new Lambertian({ 0.4f, 0.2f, 0.1f });
        obj = new Sphere({ -4.0f, 1.0f, 0.0f }, 1.0f, mat);
        hitables.push_back(obj);
        materials.push_back(mat);
    }

    {
        mat = new Metal({ 0.7f, 0.6f, 0.5f }, 0.0f);
        obj = new Sphere({ 4.0f, 1.0f, 0.0f }, 1.0f, mat);
        hitables.push_back(obj);
        materials.push_back(mat);
    }
}

int main(int argc, char *argv[]) {
    std::vector<Material *> materials;
    std::vector<Hitable *> hitables;
    RandomScene(hitables, materials);
    HitableList world(hitables.data(), static_cast<int>(hitables.size()));

    XMVECTOR lookFrom = {13.0f, 2.0f, 3.0f, 0.0f};
    XMVECTOR lookAt = {0.0f, 0.0f, 0.0f, 0.0f};
    Camera camera(lookFrom, lookAt, {0.0f, 1.0f, 0.0f, 0.0f}, XM_PIDIV4 * 0.5f, float(nx) / float(ny), 0.1f, 10.0f);

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
