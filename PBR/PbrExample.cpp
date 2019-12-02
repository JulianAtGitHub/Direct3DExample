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
, mLightsBuffer(nullptr)
, mEnvTexture(nullptr)
, mEnvTextureHeap(nullptr)
, mCamera(nullptr)
, mGUI(nullptr)
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

    mSettings = { 1, 0, LIGHT_COUNT };
    mLights[0] = { {-10.0f, 10.0f, 10.0f }, 0, { 300.0f, 300.0f, 300.0f }, 0.0f };
    mLights[1] = { { 10.0f, 10.0f, 10.0f }, 0, { 300.0f, 300.0f, 300.0f }, 0.0f };
    mLights[2] = { {-10.0f,-10.0f, 10.0f }, 0, { 300.0f, 300.0f, 300.0f }, 0.0f };
    mLights[3] = { { 10.0f,-10.0f, 10.0f }, 0, { 300.0f, 300.0f, 300.0f }, 0.0f };

    mLightsBuffer = new Render::GPUBuffer(LIGHT_COUNT * sizeof(LightCB));

    Utils::Image *image = Utils::Image::CreateFromFile("..\\..\\Models\\WoodenDoor_Ref.hdr", false);
    mEnvTexture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), 1, image->GetDXGIFormat(), D3D12_RESOURCE_STATE_COMMON);
    mEnvTextureHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    mEnvTexture->CreateSRV(mEnvTextureHeap->Allocate());

    Render::gCommand->Begin();
    Render::gCommand->UploadTexture(mEnvTexture, image->GetPixels());
    Render::gCommand->UploadBuffer(mLightsBuffer, 0, mLights, mLightsBuffer->GetBufferSize());
    Render::gCommand->End(true);

    delete image;

    mCamera = new Utils::Camera(XM_PIDIV4, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.001f, 100.0f, XMFLOAT4(0.0f, 0.0f, 3.0f, 0.0f));
    mGUI = new Utils::GUILayer(mHwnd, mWidth, mHeight);

    Utils::Scene *sphere = Utils::Model::LoadFromFile("..\\..\\Models\\Others\\sphere.obj");
    ASSERT_PRINT(sphere);

    sphere->mImages.reserve(5);
    Utils::Scene::Shape &shape = sphere->mShapes[0];

    shape.normalTex = 0; 
    sphere->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\normal.png"));

    shape.albdoTex = 1; 
    sphere->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\albedo.png"));

    shape.metalnessTex = 2; 
    sphere->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\metallic.png"));

    shape.roughnessTex = 3; 
    sphere->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\roughness.png"));

    shape.aoTex = 4; 
    sphere->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\ao.png"));

    mSphere = new PbrDrawable();
    mSphere->Initialize(sphere, mLightsBuffer, LIGHT_COUNT);
    delete sphere;

    mPbrPass = new PbrPass();
    mSkyboxPass = new SkyboxPass();

    mTimer.Reset();
}

void PbrExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    DeleteAndSetNull(mGUI);
    DeleteAndSetNull(mLightsBuffer);
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
    mGUI->OnKeyDown(key);
    switch (key) {
        case 'W': mSpeedZ = 1.0f; break;
        case 'S': mSpeedZ = -1.0f; break;
        case 'A': mSpeedX = -1.0f; break;
        case 'D': mSpeedX = 1.0f; break;
        default: break;
    }
}

void PbrExample::OnKeyUp(uint8_t key) {
    mGUI->OnKeyUp(key);
    switch (key) {
        case 'W':
        case 'S': mSpeedZ = 0.0f; break;
        case 'A':
        case 'D': mSpeedX = 0.0f; break;
        default: break;
    }
}

void PbrExample::OnChar(uint16_t cha) {
    mGUI->OnChar(cha);
}

void PbrExample::OnMouseLButtonDown(int64_t pos) {
    mGUI->OnMouseLButtonDown();
    if (mGUI->IsHovered()) {
        return;
    }

    mIsRotating = true;
    mLastMousePos = pos;
    mCurrentMousePos = pos;
}

void PbrExample::OnMouseLButtonUp(int64_t pos) {
    mGUI->OnMouseLButtonUp();
    mIsRotating = false;
}

void PbrExample::OnMouseRButtonDown(int64_t pos) {
    if (mGUI->IsHovered()) {
        return;
    }
    mGUI->OnMouseRButtonDown();
}

void PbrExample::OnMouseRButtonUp(int64_t pos) {
    mGUI->OnMouseRButtonUp();
}

void PbrExample::OnMouseMButtonDown(int64_t pos) {
    if (mGUI->IsHovered()) {
        return;
    }
    mGUI->OnMouseMButtonDown();
}

void PbrExample::OnMouseMButtonUp(int64_t pos) {
    mGUI->OnMouseMButtonUp();
}

void PbrExample::OnMouseMove(int64_t pos) {
    mGUI->OnMouseMove(pos);
    mCurrentMousePos = pos;
}

void PbrExample::OnMouseWheel(uint64_t param) {
    mGUI->OnMouseWheel(param);
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

    mSphere->Update(mCurrentFrame, *mCamera, mSettings);
    mSkyboxPass->Update(mCurrentFrame, *mCamera);

    UpdateGUI(deltaSecond);
}

void PbrExample::UpdateGUI(float second) {
    mGUI->BeginFrame(second);

    ImGui::SetNextWindowSize(ImVec2(350.0f, 300.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_Once);
    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::BeginChild("Settings");

    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "Settings");
    ImGui::Checkbox("Enable Texture", (bool *)&mSettings.enableTexture);
    ImGui::Checkbox("Enable IBL", (bool *)&mSettings.enableIBL);

    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "Material");
    ImGui::ColorEdit3("Albdo", &(mSphere->GetMaterial().albdo.x));
    ImGui::SliderFloat("Matelness", &(mSphere->GetMaterial().metalness), 0.04f, 1.0f);
    ImGui::SliderFloat("Roughness", &(mSphere->GetMaterial().roughness), 0.04f, 1.0f);

    ImGui::EndChild();

    ImGui::End();

    mGUI->EndFrame(mCurrentFrame);
}

void PbrExample::Render(void) {
    Render::gCommand->Begin();

    Render::gCommand->SetViewportAndScissor(0, 0, mWidth, mHeight);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);

    Render::gCommand->SetRenderTarget(Render::gRenderTarget[mCurrentFrame], Render::gDepthStencil);

    Render::gCommand->ClearColor(Render::gRenderTarget[mCurrentFrame]);
    Render::gCommand->ClearDepth(Render::gDepthStencil);

    mPbrPass->PreviousRender();
    mPbrPass->Render(mCurrentFrame, mSphere);
    mSkyboxPass->Render(mCurrentFrame, mEnvTextureHeap, 0);
    mGUI->Draw(mCurrentFrame, Render::gRenderTarget[mCurrentFrame]);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);

    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}
