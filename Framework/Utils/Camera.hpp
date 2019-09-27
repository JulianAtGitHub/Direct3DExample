#pragma once

namespace Utils {

class Camera {
public:
    Camera( const float fov, const float aspectRatio, const float near, const float far, 
            const XMFLOAT4 eye, 
            const XMFLOAT4 lookAt = { 0.0f, 0.0f, 0.0f, 0.0f }, 
            const XMFLOAT4 up = { 0.0f, 1.0f, 0.0f, 0.0f })
    : mFov(fov)
    , mAspectRatio(aspectRatio)
    , mNear(near)
    , mFar(far)
    , mFocalLength(0.0f)
    , mFNumber(0.0f)
    {
        mPosition = XMLoadFloat4(&eye);
        mDirection = XMVector3Normalize(XMLoadFloat4(&lookAt) - mPosition);
        mUp = XMVector3Normalize(XMLoadFloat4(&up));
        mRight = XMVector3Normalize(XMVector3Cross(mDirection, mUp));
        mProject = XMMatrixPerspectiveFovRH(fov, aspectRatio, near, far);
        UpdateMatrixs();
    }

    ~Camera(void) {

    }

    INLINE void SetLensParams(float fNumber, float focalLen) { mFNumber = fNumber; mFocalLength = focalLen; }
    INLINE float GetFocalLength(void) const { return mFocalLength; }
    INLINE float GetLensRadius(void) { return mFocalLength > 0.0f ? mFocalLength / (2.0f * mFNumber) : 0.0f; }

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
    float       mFocalLength;
    float       mFNumber;

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

INLINE void Camera::UpdateMatrixs(void) {
    mView = XMMatrixLookToRH(mPosition, mDirection, mUp); 

    mW = XMVectorScale(mDirection, mFar);

    mU = XMVector3Normalize(XMVector3Cross(mW, mUp));
    mU = XMVectorScale(mU, mFar * tanf(mFov * 0.5f) * mAspectRatio);

    mV = XMVector3Normalize(XMVector3Cross(mU, mW));
    mV = XMVectorScale(mV, mFar * tanf(mFov * 0.5f));
}

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
