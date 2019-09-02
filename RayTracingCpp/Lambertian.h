#pragma once

#include "Material.h"

class Lambertian : public Material {
public:
    Lambertian(const XMVECTOR &albedo)
    {
        mAlbedo = albedo;
    }
    ~Lambertian(void) {
    
    }

    virtual bool Scatter(const Ray &in, const Hitable::Record &record, XMVECTOR &attenuation, Ray &scatter);

private:
    XMVECTOR    mAlbedo;
};

INLINE bool Lambertian::Scatter(const Ray &in, const Hitable::Record &record, XMVECTOR &attenuation, Ray &scatter) {
    XMVECTOR target = record.p + record.n + RandomInUnitSphere();
    scatter = Ray(record.p, target - record.p);
    attenuation = mAlbedo;
    return true;
}

