#include "stdafx.h"
#include "PbrExample.h"
#include "PbrDrawable.h"
#include "PbrPass.h"
#include "SkyboxPass.h"

std::string PbrExample::WindowTitle = "PBR Example";

PbrExample::PbrExample(void)
: Utils::AnExample()
, mWidth(0)
, mHeight(0)
, mCurrentFrame(0)
, mSpeedX(0)
, mSpeedZ(0)
, mIsRotating(false)
, mLastMousePos(0)
, mCurrentMousePos(0)
, mEnvTexture(nullptr)
, mEnvTextureHeap(nullptr)
, mCamera(nullptr)
, mSphere(nullptr)
, mPbrPass(nullptr)
, mSkyboxPass(nullptr)
{
    for (auto &fence : mFenceValues) {
        fence = 1;
    }
}

PbrExample::~PbrExample(void) {

}

void PbrExample::Init(HWND hwnd) {
    Utils::AnExample::Init(hwnd);
    WINDOWINFO windowInfo;
    GetWindowInfo(mHwnd, &windowInfo);
    mWidth = windowInfo.rcClient.right - windowInfo.rcClient.left;
    mHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;

    SetWindowText(mHwnd, WindowTitle.c_str());
    Render::Initialize(mHwnd);
    Utils::CreateMipsGenerator();
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    Utils::Image *image = Utils::Image::CreateFromFile("..\\..\\Models\\WoodenDoor_Ref.hdr", false);
    mEnvTexture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), 1, image->GetDXGIFormat(), D3D12_RESOURCE_STATE_COMMON);
    mEnvTextureHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    mEnvTexture->CreateSRV(mEnvTextureHeap->Allocate());

    Render::gCommand->Begin();
    Render::gCommand->UploadTexture(mEnvTexture, image->GetPixels());
    Render::gCommand->End(true);

    delete image;

    mCamera = new Utils::Camera(XM_PIDIV4, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.1f, 1000.0f, XMFLOAT4(0.0f, 0.0f, 5.0f, 0.0f));

    Utils::Scene *sphere = Utils::Model::LoadFromFile("..\\..\\Models\\Others\\sphere.obj");
    ASSERT_PRINT(sphere);
    mSphere = new PbrDrawable(sphere);
    delete sphere;

    mPbrPass = new PbrPass();
    mSkyboxPass = new SkyboxPass();

    mTimer.Reset();
}

void PbrExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    DeleteAndSetNull(mEnvTextureHeap);
    DeleteAndSetNull(mEnvTexture);
    DeleteAndSetNull(mCamera);
    DeleteAndSetNull(mSphere);
    DeleteAndSetNull(mPbrPass);
    DeleteAndSetNull(mSkyboxPass);

    Utils::DestroyMipsGenerator();
    Render::Terminate();
}

void PbrExample::OnKeyDown(uint8_t key) {
    switch (key) {
        case 'W': mSpeedZ = 1.0f; break;
        case 'S': mSpeedZ = -1.0f; break;
        case 'A': mSpeedX = -1.0f; break;
        case 'D': mSpeedX = 1.0f; break;
        default: break;
    }
}

void PbrExample::OnKeyUp(uint8_t key) {
    switch (key) {
        case 'W':
        case 'S': mSpeedZ = 0.0f; break;
        case 'A':
        case 'D': mSpeedX = 0.0f; break;
        default: break;
    }
}

void PbrExample::OnMouseLButtonDown(int64_t pos) {
    mIsRotating = true;
    mLastMousePos = pos;
    mCurrentMousePos = pos;
}

void PbrExample::OnMouseLButtonUp(int64_t pos) {
    mIsRotating = false;
}

void PbrExample::OnMouseMove(int64_t pos) {
    mCurrentMousePos = pos;
}

void PbrExample::Update(void) {
    mTimer.Tick();
    float deltaSecond = static_cast<float>(mTimer.GetElapsedSeconds());

    if (mIsRotating) {
        int32_t deltaX = GET_X_LPARAM(mCurrentMousePos) - GET_X_LPARAM(mLastMousePos);
        int32_t deltaY = GET_Y_LPARAM(mCurrentMousePos) - GET_Y_LPARAM(mLastMousePos);
        if (deltaX) {
            mCamera->RotateY(XMConvertToRadians(-deltaX * 0.2f));
        }
        if (deltaY) {
            mCamera->RotateX(XMConvertToRadians(-deltaY * 0.2f));
        }

        mLastMousePos = mCurrentMousePos;
    }

    if (mSpeedZ != 0.0f) {
        mCamera->MoveForward(deltaSecond * mSpeedZ * 0.5f);
    }
    if (mSpeedX != 0.0f) {
        mCamera->MoveRight(deltaSecond * mSpeedX * 0.5f);
    }

    mCamera->UpdateMatrixs();
    XMMATRIX model = XMMatrixIdentity();
    XMMATRIX view = mCamera->GetViewMatrix();
    XMMATRIX proj = mCamera->GetProjectMatrix();

    XMFLOAT4X4 mvp;
    XMStoreFloat4x4(&mvp, XMMatrixMultiply(XMMatrixMultiply(model, view), proj));
    mSphere->Update(mCurrentFrame, mvp);

    XMFLOAT4X4 viewMat;
    XMFLOAT4X4 projMat;
    XMStoreFloat4x4(&viewMat, view);
    XMStoreFloat4x4(&projMat, proj);
    mSkyboxPass->Update(mCurrentFrame, viewMat, projMat);
}

void PbrExample::Render(void) {
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);

    Render::gCommand->Begin();

    Render::gCommand->SetViewportAndScissor(0, 0, mWidth, mHeight);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);

    Render::gCommand->SetRenderTarget(Render::gRenderTarget[mCurrentFrame], Render::gDepthStencil);

    Render::gCommand->ClearColor(Render::gRenderTarget[mCurrentFrame]);
    Render::gCommand->ClearDepth(Render::gDepthStencil);

    mPbrPass->PreviousRender();
    mPbrPass->Render(mCurrentFrame, mSphere);
    mSkyboxPass->Render(mCurrentFrame, mEnvTextureHeap, 0);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);

    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
}
