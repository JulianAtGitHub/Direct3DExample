#pragma once

#include "Material.h"

class Metal : public Material {
public:
    Metal(const XMVECTOR &albedo, float fuzz)
    {
        mAlbedo = albedo;
        mFuzz = std::max(std::min(fuzz, 1.0f), 0.0f);
    }
    ~Metal(void) {
    
    }

    virtual bool Scatter(const Ray &in, const Hitable::Record &record, XMVECTOR &attenuation, Ray &scatter);

private:
    XMVECTOR    mAlbedo;
    float       mFuzz;
};

bool Metal::Scatter(const Ray &in, const Hitable::Record &record, XMVECTOR &attenuation, Ray &scatter) {
    XMVECTOR reflected = XMVector3Reflect(in.Direction(), record.n);
    scatter = Ray(record.p, reflected + (RandomInUnitSphere() * mFuzz));
    attenuation = mAlbedo;
    return (XMVectorGetX(XMVector3Dot(reflected, record.n)) > 0.0f);
}

