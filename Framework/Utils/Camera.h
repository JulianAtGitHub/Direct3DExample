#pragma once

namespace Utils {

class Camera {
public:
    Camera(float fov, float aspectRatio, float near, float far, 
           const XMFLOAT4 &eye, const XMFLOAT4 &lookAt, const XMFLOAT4 &right);
    ~Camera(void);

    INLINE XMVECTOR GetPosition(void) const { return mPosition; };

    INLINE XMMATRIX GetCombinedMatrix(void) { return mView * mProject; }
    INLINE XMMATRIX GetViewMatrix(void) const { return mView; }
    INLINE XMMATRIX GetProjectMatrix(void) const { return mProject; }

    INLINE void UpdateMatrixs(void) { mView = XMMatrixLookToRH(mPosition, mDirection, mUp); }

    INLINE void MoveForward(float distance) { mPosition = mPosition + (distance * mDirection); }
    INLINE void MoveRight(float distance) { mPosition = mPosition + (distance * mRight); }

    void RotateY(float angle);
    void RotateX(float angle);

private:
    XMVECTOR    mPosition;
    XMVECTOR    mDirection;
    XMVECTOR    mUp;
    XMVECTOR    mRight;
    XMMATRIX    mView;
    XMMATRIX    mProject;
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
