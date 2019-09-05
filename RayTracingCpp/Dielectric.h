#pragma once

#include "Material.h"

class Dielectric : public Material {
public:
    Dielectric(float refIdx)
    : mRefIdx(refIdx)
    {

    }
    ~Dielectric(void) {

    }

    virtual bool Scatter(const Ray &in, const Hitable::Record &record, XMVECTOR &attenuation, Ray &scatter);

private:
    INLINE static float Schlick(float cosine, float refIdx) {
        float r0 = (1.0f - refIdx) / (1.0f + refIdx);
        r0 = r0 * r0;
        return r0 + (1.0f - r0) * std::powf((1.0f - cosine), 5.0f);
    }

    float       mRefIdx; // refractive indices
};

INLINE bool Dielectric::Scatter(const Ray &in, const Hitable::Record &record, XMVECTOR &attenuation, Ray &scatter) {
    XMVECTOR reflected = XMVector3Reflect(in.Direction(), record.n);
    attenuation = {0.90f, 0.99f, 0.99f};

    XMVECTOR outwardNormal;
    float refractionIndex;
    float cosine;
    float IDotN = XMVectorGetX(XMVector3Dot(in.Direction(), record.n));
    if (IDotN > 0.0f) {
        outwardNormal = -record.n;
        refractionIndex = mRefIdx;
        cosine = mRefIdx * IDotN;
    } else {
        outwardNormal = record.n;
        refractionIndex = 1.0f / mRefIdx;
        cosine = -IDotN;
    }
    float reflectPercentage;
    XMVECTOR refracted = XMVector3Refract(in.Direction(), outwardNormal, refractionIndex);
    if (XMVector3Equal(refracted, g_XMZero)) {
        reflectPercentage = 1.0f;
    } else {
        reflectPercentage = Schlick(cosine, mRefIdx);
    }

    if (RandomUnit() < reflectPercentage) {
        scatter = Ray(record.p, reflected);
    } else {
        scatter = Ray(record.p, refracted);
    }

    return true;
}

