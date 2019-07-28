#include "stdafx.h"
#include "Camera.h"

namespace Utils {

Camera::Camera(float fov, float aspectRatio, float near, float far, const XMFLOAT4 &eye, const XMFLOAT4 &lookAt, const XMFLOAT4 &right)
{
    mPosition = XMLoadFloat4(&eye);
    mDirection = XMVector3Normalize(XMLoadFloat4(&lookAt) - mPosition);
    mRight = XMVector3Normalize(XMLoadFloat4(&right));
    mUp = { 0.0f, 1.0f, 0.0f, 0.0f };
    mProject = XMMatrixPerspectiveFovRH(fov, aspectRatio, near, far);
    UpdateMatrixs();
}

Camera::~Camera(void) {

}

}