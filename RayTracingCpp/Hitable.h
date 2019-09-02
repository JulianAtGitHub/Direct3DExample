#pragma once

#include "Ray.h"

class Hitable {
public:
    struct Record {
        float t;
        XMVECTOR p;
        XMVECTOR n;
    };

    virtual bool Hit(const Ray &ray, float tMin, float tMax, Record &record) = 0;

};
