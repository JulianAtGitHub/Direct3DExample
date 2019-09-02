#pragma once

#include "Hitable.h"

class Material;

class Sphere : public Hitable {
public:
    Sphere(const XMVECTOR& center, float radius, Material *material)
    : mRadius(radius)
    , mMaterial(material)
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
    Material   *mMaterial;
};

INLINE void Sphere::RecordHit(const Ray& ray, float t, Record& record) {
    record.t = t;
    record.p = ray.PointAt(t);
    record.n = XMVector3Normalize(record.p - mCenter);
    record.mat = mMaterial;
}

INLINE bool Sphere::Hit(const Ray& ray, float tMin, float tMax, Record& record) {
    XMVECTOR oc = ray.Origin() - mCenter;
    float a = XMVectorGetX(XMVector3Dot(ray.Direction(), ray.Direction()));
    float b = XMVectorGetX(XMVector3Dot(ray.Direction(), oc));
    float c = XMVectorGetX(XMVector3Dot(oc, oc)) - mRadius * mRadius;
    float discriminant = b * b - a * c;
    if (discriminant > 0.0f) {
        discriminant = std::sqrt(discriminant);
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

