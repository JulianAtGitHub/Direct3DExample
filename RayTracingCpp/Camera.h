#pragma once

#include "Ray.h"

class Camera {
public:
    Camera(XMVECTOR lookFrom, XMVECTOR lookAt, XMVECTOR vup, float vFov, float aspect) 
    {
        XMVECTOR u, v, w;
        float halfHeight = std::tanf(vFov / 2.0f);
        float halfWidth = aspect * halfHeight;
        mOrigin = lookFrom;
        w = XMVector3Normalize(lookFrom - lookAt);
        u = XMVector3Normalize(XMVector3Cross(vup, w));
        v = XMVector3Normalize(XMVector3Cross(w, u));
        mBottomLeft = (mOrigin - w) - (u * halfWidth) - (v * halfHeight);
        mHorizontal = u * (halfWidth * 2.0f);
        mVertical = v * (halfHeight * 2.0f);
    }
    ~Camera(void) {
    
    }

    INLINE Ray GenRay(float u, float v) {
        return Ray(mOrigin, (mBottomLeft + (mHorizontal * u) + (mVertical * v)) - mOrigin);
    }

private:
    XMVECTOR mBottomLeft;
    XMVECTOR mHorizontal;
    XMVECTOR mVertical;
    XMVECTOR mOrigin;

};

