#pragma once

#include "Ray.h"

class Camera {
public:
    Camera(XMVECTOR lookFrom, XMVECTOR lookAt, XMVECTOR vup, float vFov, float aspect, float aperture, float focalLength) 
    {
        mOrigin = lookFrom;
        mLenRadius = aperture * 0.5f;
        float halfHeight = std::tanf(vFov * 0.5f);
        float halfWidth = aspect * halfHeight;
        mW = XMVector3Normalize(lookFrom - lookAt);
        mU = XMVector3Normalize(XMVector3Cross(vup, mW));
        mV = XMVector3Normalize(XMVector3Cross(mW, mU));
        mBottomLeft = mOrigin - (mW * focalLength) - (mU * (halfWidth * focalLength)) - (mV * (halfHeight * focalLength));
        mHorizontal = mU * (halfWidth * 2.0f * focalLength);
        mVertical = mV * (halfHeight * 2.0f * focalLength);
    }
    ~Camera(void) {
    
    }

    INLINE Ray GenRay(float u, float v) {
        XMVECTOR rd = RandomInUnitDisk() * mLenRadius;
        XMVECTOR offset = mU * XMVectorGetX(rd) + mV * XMVectorGetY(rd);
        return Ray(mOrigin + offset, (mBottomLeft + (mHorizontal * u) + (mVertical * v)) - mOrigin - offset);
    }

private:
    XMVECTOR mBottomLeft;
    XMVECTOR mHorizontal;
    XMVECTOR mVertical;
    XMVECTOR mOrigin;
    XMVECTOR mU, mV, mW;
    float mLenRadius;

};

