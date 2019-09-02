
#pragma once

class Ray {
public:
    Ray(const XMVECTOR& origin, const XMVECTOR& direction)
    {
        mOrigin = origin;
        mDirection = XMVector3Normalize(direction);
    }
    ~Ray(void) {

    }

    INLINE XMVECTOR Origin(void) const { return mOrigin; }
    INLINE XMVECTOR Direction(void) const { return mDirection; }
    INLINE XMVECTOR PointAt(float t) const { return mOrigin + mDirection * t; }

private:
    XMVECTOR mOrigin;
    XMVECTOR mDirection;
};
