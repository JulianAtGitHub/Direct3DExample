#pragma once

#include "Hitable.h"

class Sphere : public Hitable {
public:
    Sphere(const XMVECTOR& center, float radius)
    : mRadius(radius)
    {
        mCenter = center;
    }
    ~Sphere(void) {

    }

    INLINE XMVECTOR Center(void) const { return mCenter; }
    INLINE float Radius(void) const { return mRadius; }

    void RecordHit(const Ray& ray, float t, Record& record);
    virtual bool Hit(const Ray &ray, float tMin, float tMax, Record &record);

private:
    XMVECTOR    mCenter;
    float       mRadius;
};

INLINE void Sphere::RecordHit(const Ray& ray, float t, Record& record) {
    record.t = t;
    record.p = ray.PointAt(t);
    record.n = (record.p - mCenter) / mRadius;
}

INLINE bool Sphere::Hit(const Ray& ray, float tMin, float tMax, Record& record) {
    XMVECTOR oc = ray.Origin() - mCenter;
    float a = XMVectorGetX(XMVector3Dot(ray.Direction(), ray.Direction()));
    float b = XMVectorGetX(XMVector3Dot(ray.Direction(), oc));
    float c = XMVectorGetX(XMVector3Dot(oc, oc)) - mRadius * mRadius;
    float discriminant = b * b - a * c;
    if (discriminant > 0.0f) {
        discriminant = sqrt(discriminant);
        float t = (-b - discriminant) / a;
        if (t < tMax && t > tMin) {
            RecordHit(ray, t, record);
            return true;
        }
        t = (-b + discriminant) / a;
        if (t < tMax && t > tMin) {
            RecordHit(ray, t, record);
            return true;
        }
    }
    return false;
}

