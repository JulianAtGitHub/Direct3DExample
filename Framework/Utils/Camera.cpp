#include "stdafx.h"
#include "Camera.h"

namespace Utils {

Camera::Camera(float fov, float aspectRatio, float near, float far, const XMFLOAT4 &eye, const XMFLOAT4 &lookAt)
: mFov(fov)
, mAspectRatio(aspectRatio)
, mNear(near)
, mFar(near)
, mFocalLength(0.0f)
, mFNumber(0.0f)
{
    mPosition = XMLoadFloat4(&eye);
    mDirection = XMVector3Normalize(XMLoadFloat4(&lookAt) - mPosition);
    mUp = { 0.0f, 1.0f, 0.0f, 0.0f };
    mRight = XMVector3Normalize(XMVector3Cross(mDirection, mUp));
    mProject = XMMatrixPerspectiveFovRH(fov, aspectRatio, near, far);
    UpdateMatrixs();
}

Camera::Camera(float fov, float aspectRatio, float near, float far, const XMFLOAT4 &eye, const XMFLOAT4 &lookAt, const XMFLOAT4 &up)
: mFov(fov)
, mAspectRatio(aspectRatio)
, mNear(near)
, mFar(near)
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

Camera::~Camera(void) {

}

void Camera::UpdateMatrixs(void) { 
    mView = XMMatrixLookToRH(mPosition, mDirection, mUp); 

    mW = XMVectorScale(mDirection, mFar);

    mU = XMVector3Normalize(XMVector3Cross(mW, mUp));
    mU = XMVectorScale(mU, mFar * tanf(mFov * 0.5f) * mAspectRatio);

    mV = XMVector3Normalize(XMVector3Cross(mU, mW));
    mV = XMVectorScale(mV, mFar * tanf(mFov * 0.5f));
}

}