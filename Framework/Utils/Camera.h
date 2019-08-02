#pragma once

namespace Utils {

class Camera {
public:
    Camera(float fov, float aspectRatio, float near, float far, 
           const XMFLOAT4 &eye, const XMFLOAT4 &lookAt);
    Camera(float fov, float aspectRatio, float near, float far, 
           const XMFLOAT4 &eye, const XMFLOAT4 &lookAt, const XMFLOAT4 &up);
    ~Camera(void);

    INLINE XMVECTOR GetPosition(void) const { return mPosition; };
    INLINE XMVECTOR GetDirection(void) const { return mDirection; };
    INLINE XMVECTOR GetUp(void) const { return mUp; };

    INLINE XMVECTOR GetU(void) const { return mU; };
    INLINE XMVECTOR GetV(void) const { return mV; };
    INLINE XMVECTOR GetW(void) const { return mW; };

    INLINE XMMATRIX GetCombinedMatrix(void) { return mView * mProject; }
    INLINE XMMATRIX GetViewMatrix(void) const { return mView; }
    INLINE XMMATRIX GetProjectMatrix(void) const { return mProject; }

    INLINE void MoveForward(float distance) { mPosition = mPosition + (distance * mDirection); }
    INLINE void MoveRight(float distance) { mPosition = mPosition + (distance * mRight); }

    void UpdateMatrixs(void);

    void RotateY(float angle);
    void RotateX(float angle);

private:
    float       mFov;
    float       mAspectRatio;
    float       mNear;
    float       mFar;

    XMVECTOR    mPosition;
    XMVECTOR    mDirection;
    XMVECTOR    mUp;
    XMVECTOR    mRight;
    XMMATRIX    mView;
    XMMATRIX    mProject;

    XMVECTOR    mU;
    XMVECTOR    mV;
    XMVECTOR    mW;
};

INLINE void Camera::RotateY(float angle) {
    XMVECTOR quaternion = XMQuaternionRotationAxis(mUp, angle);
    mDirection = XMVector3Rotate(mDirection, quaternion);
    mRight = XMVector3Rotate(mRight, quaternion);
}

INLINE void Camera::RotateX(float angle) {
    XMVECTOR quaternion = XMQuaternionRotationAxis(mRight, angle);
    mDirection = XMVector3Rotate(mDirection, quaternion);
}

}
