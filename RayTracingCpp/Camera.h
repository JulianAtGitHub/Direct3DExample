#pragma once

#include "Ray.h"

class Camera {
public:
    Camera(void) 
    {
        mBottomLeft = {-2.0f, -1.0f, -1.0f, 0.0f};
        mHorizontal = {4.0f, 0.0f, 0.0f, 0.0f};
        mVertical = {0.0f, 2.0f, 0.0f, 0.0f};
        mOrigin = {0.0f, 0.0f, 0.0f, 0.0f};
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

