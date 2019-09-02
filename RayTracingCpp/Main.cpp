
#include "pch.h"
#include "Ray.h"
#include "Sphere.h"
#include "HitableList.h"
#include "Camera.h"

static std::mt19937 rang;
static std::uniform_real_distribution<float> rangDist(0.0f); // dist from [0.0 ~ 1.0)

static constexpr int nx = 400;
static constexpr int ny = 200;
static constexpr int ns = 100;

XMVECTOR RandomInUnitSphere(void) {
    static XMVECTOR one = {1.0f, 1.0f, 1.0f, 0.0f};
    XMVECTOR p;
    do {
        p = {rangDist(rang), rangDist(rang), rangDist(rang), 0.0f};
        p = p * 2.0f - one;
    } while(XMVectorGetX(XMVector3Dot(p, p)) >= 1.0f);
    return p;
}

XMVECTOR CalculateColor(const Ray& ray, Hitable *world) {
    static XMVECTOR white = {1.0f, 1.0f, 1.0f, 0.0f};
    static XMVECTOR blue = {0.5f, 0.7f, 1.0f, 0.0f};
    Hitable::Record record;
    if (world->Hit(ray, 0.001f, 1e+38f, record)) {
        XMVECTOR target = record.p + record.n + RandomInUnitSphere();
        return 0.5f * CalculateColor(Ray(record.p, target - record.p), world);
    } else {
        XMVECTOR direction = XMVector3Normalize(ray.Direction());
        float t = (XMVectorGetY(direction) + 1.0f) * 0.5f;
        return white * (1.0f - t) + blue * t;
    }
}

int main(int argc, char *argv[]) {
    Hitable* list[2] = {
        new Sphere({0.0f, 0.0f, -1.0f}, 0.5f),
        new Sphere({0, -100.5f, -1.0f}, 100.0f)
    };
    HitableList world(list, 2);
    Camera camera;

    std::stringstream ss;
    ss << "P3\n" << nx << " " << ny << "\n255\n";
    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            XMVECTOR col = {0.0f, 0.0f, 0.0f};
            for (int s = 0; s < ns; ++s) {
                float u = (i + rangDist(rang) - 0.5f) / float(nx);
                float v = (j + rangDist(rang) - 0.5f) / float(ny);
                col += CalculateColor(camera.GenRay(u, v), &world);
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

    for (int i = 0; i < 2; ++i) {
        delete list[i];
    }

    // write to file
    std::string file("output.ppm");
    std::ofstream ofs;
    ofs.open(file);
    ofs << ss.str();
    ofs.close();

    return 0;
}
