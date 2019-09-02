#pragma once

#include "Hitable.h"
#include "Ray.h"

class Material {
public:
    virtual bool Scatter(const Ray &in, const Hitable::Record &record, XMVECTOR &attenuation, Ray &scatter) = 0;
};
