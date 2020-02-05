#include "stdafx.h"
#include "PbrExample.h"
#include "PbrDrawable.h"
#include "PbrPass.h"
#include "SkyboxPass.h"
#include "IrradiancePass.h"
#include "PrefilteredEnvPass.h"
#include "BRDFIntegrationPass.h"

std::string PbrExample::WindowTitle = "PBR Example";

PbrExample::PbrExample(void)
: Utils::AnExample()
, mWidth(0)
, mHeight(0)
, mCurrentFrame(0)
, mSpeedX(0)
, mSpeedZ(0)
, mSpeedBoost(1.0f)
, mIsRotating(false)
, mLastMousePos(0)
, mCurrentMousePos(0)
, mDirtyFlag(false)
, mLightsBuffer(nullptr)
, mEnvTexture(nullptr)
, mIrrTexture(nullptr)
, mBlurredEnvTexture(nullptr)
, mBRDFLookupTexture(nullptr)
, mTextureHeap(nullptr)
, mCamera(nullptr)
, mGUI(nullptr)
, mDrawIndex(0)
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
    Render::Initialize(mHwnd, Render::Enable8xMSAA);
    Utils::CreateMipsGenerator();
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    mAppSettings = { true, false, false, false, false, true, true, 3 };
    mSettings = { LIGHT_COUNT, { 0.05f, 0.05f, 0.05f } };
    mLights[0] = { {  0.0f,  0.0f, 10.0f }, 0, { 100.0f, 100.0f, 100.0f }, 0.0f };
    mLights[1] = { {-10.0f, 10.0f, 10.0f }, 0, { 100.0f, 100.0f, 100.0f }, 0.0f };
    mLights[2] = { { 10.0f, 10.0f, 10.0f }, 0, { 100.0f, 100.0f, 100.0f }, 0.0f };
    mLights[3] = { {-10.0f,-10.0f, 10.0f }, 0, { 100.0f, 100.0f, 100.0f }, 0.0f };
    mLights[4] = { { 10.0f,-10.0f, 10.0f }, 0, { 100.0f, 100.0f, 100.0f }, 0.0f };

    mLightsBuffer = new Render::GPUBuffer(LIGHT_COUNT * sizeof(LightCB));

    mTextureHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);

    Utils::Image *image = Utils::Image::CreateFromFile("..\\..\\Models\\WoodenDoor_Ref.hdr", false);
    mEnvTexture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), 1, image->GetDXGIFormat(), D3D12_RESOURCE_STATE_COMMON);
    mEnvTexture->CreateSRV(mTextureHeap->Allocate());

    uint32_t irrWidth = mEnvTexture->GetWidth() >> 1;
    uint32_t irrHeight = mEnvTexture->GetHeight() >> 1;
    DXGI_FORMAT irrFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    if (Render::gTypedUAVLoadSupport_R16G16B16A16_FLOAT) {
        irrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    }
    uint32_t irrPitch = Render::BytesPerPixel(irrFormat) * irrWidth;
    mIrrTexture = new Render::PixelBuffer(irrPitch, irrWidth, irrHeight, 1, irrFormat, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mIrrTexture->CreateSRV(mTextureHeap->Allocate());

    DXGI_FORMAT blurredFormat = irrFormat;
    uint32_t blurredWidth = 256;
    uint32_t blurredPitch = Render::BytesPerPixel(blurredFormat) * blurredWidth;
    uint32_t blurredMips = 6;
    mBlurredEnvTexture = new Render::PixelBuffer(blurredPitch, blurredWidth, blurredWidth >> 1, blurredMips, blurredFormat, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mBlurredEnvTexture->CreateSRV(mTextureHeap->Allocate());

    DXGI_FORMAT brdfFormat = DXGI_FORMAT_R32G32_FLOAT;
    if (Render::gTypedUAVLoadSupport_R16G16_FLOAT) {
        brdfFormat = DXGI_FORMAT_R16G16_FLOAT;
    }
    uint32_t brdfSize = 512;
    uint32_t brdfPitch = Render::BytesPerPixel(brdfFormat) * brdfSize;
    mBRDFLookupTexture = new Render::PixelBuffer(brdfPitch, brdfSize, brdfSize, 1, brdfFormat, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mBRDFLookupTexture->CreateSRV(mTextureHeap->Allocate());

    Render::gCommand->Begin();
    Render::gCommand->UploadTexture(mEnvTexture, image->GetPixels());
    Render::gCommand->UploadBuffer(mLightsBuffer, 0, mLights, mLightsBuffer->GetBufferSize());
    Render::gCommand->End(true);

    delete image;

    IrradiancePass *irrPass = new IrradiancePass();
    irrPass->Dispatch(mEnvTexture, mIrrTexture);
    delete irrPass;

    PrefilteredEnvPass *pfePass = new PrefilteredEnvPass();
    pfePass->Dispatch(mEnvTexture, mBlurredEnvTexture);
    delete pfePass;

    BRDFIntegrationPass *brdfPass = new BRDFIntegrationPass();
    brdfPass->Dispatch(mBRDFLookupTexture);
    delete brdfPass;

    mCamera = new Utils::Camera(XM_PIDIV4, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.001f, 100.0f, XMFLOAT4(0.0f, 0.0f, 5.0f, 0.0f));
    mGUI = new Utils::GUILayer(mHwnd, mWidth, mHeight);
    mGUI->AddImage(mEnvTexture);
    mGUI->AddImage(mIrrTexture);
    mGUI->AddImage(mBlurredEnvTexture);
    mGUI->AddImage(mBRDFLookupTexture);

    mPbrPass = new PbrPass();
    mSkyboxPass = new SkyboxPass();

    // Sphere
    {
        Utils::Scene *model = Utils::Model::LoadFromFile("..\\..\\Models\\Others\\sphere.obj");
        ASSERT_PRINT(model);

        model->mImages.reserve(5);
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\normal.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\albedo.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\metallic.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\roughness.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Rusted\\ao.png"));

        for (auto & shape : model->mShapes) {
            shape.normalTex = 0; 
            shape.albdoTex = 1; 
            shape.metalnessTex = 2; 
            shape.roughnessTex = 3; 
            shape.aoTex = 4; 
        }

        PbrDrawable *drawable = new PbrDrawable();
        drawable->Initialize("Sphere", model, mLightsBuffer, LIGHT_COUNT, mIrrTexture, mBlurredEnvTexture, mBRDFLookupTexture);
        mDrawables.push_back(drawable);

        delete model;
    }

    // Monkey
    {
        Utils::Scene *model = Utils::Model::LoadFromFile("..\\..\\Models\\Others\\monkey.obj");
        ASSERT_PRINT(model);

        model->mImages.reserve(5);
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Gold\\normal.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Gold\\albedo.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Gold\\metallic.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Gold\\roughness.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Gold\\ao.png"));

        for (auto & shape : model->mShapes) {
            shape.normalTex = 0; 
            shape.albdoTex = 1; 
            shape.metalnessTex = 2; 
            shape.roughnessTex = 3; 
            shape.aoTex = 4; 
        }

        PbrDrawable *drawable = new PbrDrawable();
        drawable->Initialize("Monkey", model, mLightsBuffer, LIGHT_COUNT, mIrrTexture, mBlurredEnvTexture, mBRDFLookupTexture);
        mDrawables.push_back(drawable);

        delete model;
    }

    // Dragon
    {
        Utils::Scene *model = Utils::Model::LoadFromFile("..\\..\\Models\\Others\\dragon.obj");
        ASSERT_PRINT(model);

        model->mImages.reserve(5);
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Marble\\normal.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Marble\\albedo.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Marble\\metallic.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Marble\\roughness.png"));
        model->mImages.push_back(Utils::Image::CreateFromFile("..\\..\\Models\\PBR\\Marble\\ao.png"));

        for (auto & shape : model->mShapes) {
            shape.normalTex = 0; 
            shape.albdoTex = 1; 
            shape.metalnessTex = 2; 
            shape.roughnessTex = 3; 
            shape.aoTex = 4; 
        }

        PbrDrawable *drawable = new PbrDrawable();
        drawable->Initialize("Dragon", model, mLightsBuffer, LIGHT_COUNT, mIrrTexture, mBlurredEnvTexture, mBRDFLookupTexture);
        mDrawables.push_back(drawable);

        delete model;
    }

    mTimer.Reset();
}

void PbrExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    DeleteAndSetNull(mGUI);
    DeleteAndSetNull(mLightsBuffer);
    DeleteAndSetNull(mTextureHeap);
    DeleteAndSetNull(mEnvTexture);
    DeleteAndSetNull(mIrrTexture);
    DeleteAndSetNull(mBlurredEnvTexture);
    DeleteAndSetNull(mBRDFLookupTexture);
    DeleteAndSetNull(mCamera);
    for (auto drawable : mDrawables) {
        delete drawable;
    }
    mDrawables.clear();
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
        case 0x10: mSpeedBoost = 5.0f; break; // shift
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
        case 0x10: mSpeedBoost = 1.0f; break; // shift
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
            mCamera->RotateY(XMConvertToRadians(-deltaX * 0.1f));
        }
        if (deltaY) {
            mCamera->RotateX(XMConvertToRadians(-deltaY * 0.1f));
        }

        mLastMousePos = mCurrentMousePos;
    }

    if (mSpeedZ != 0.0f) {
        mCamera->MoveForward(deltaSecond * mSpeedZ * mSpeedBoost);
    }
    if (mSpeedX != 0.0f) {
        mCamera->MoveRight(deltaSecond * mSpeedX * mSpeedBoost);
    }

    mCamera->UpdateMatrixs();

    for (auto drawable : mDrawables) {
        drawable->Update(mCurrentFrame, *mCamera, mSettings);
    }
    mSkyboxPass->Update(mCurrentFrame, *mCamera);

    UpdateGUI(deltaSecond);
}

void PbrExample::UpdateGUI(float second) {
    mGUI->BeginFrame(second);

    ImGui::SetNextWindowSize(ImVec2(350.0f, 420.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_Once);
    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::BeginChild("Options");
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "Application");
            ImGui::Checkbox("Enable Skybox", &mAppSettings.enableSkybox);
            ImGui::Checkbox("Environment Image", &mAppSettings.showEnvironmentImage);
            ImGui::Checkbox("Irradiance Image", &mAppSettings.showIrradianceImage);
            ImGui::Checkbox("Prefiltered Image", &mAppSettings.showPrefilteredImage);
            ImGui::Checkbox("BRDF Lookup Image", &mAppSettings.showBRDFLookupImage);

            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "Render");

            constexpr uint32_t MODEL_COUNT = 8;
            static const char* models[MODEL_COUNT] = { };
            for (size_t i = 0; i < mDrawables.size(); ++i) {
                models[i] = mDrawables[i]->GetName().c_str();
            }
            ImGui::Combo("Model", (int *)&mDrawIndex, models, static_cast<int>(mDrawables.size()));

            const static char* lightCountItems[] = {"0", "1", "2", "3", "4", "5"};
            static int curLightCount = IM_ARRAYSIZE(lightCountItems) - 1;
            ImGui::Combo("Point Lights", &curLightCount, lightCountItems, IM_ARRAYSIZE(lightCountItems));
            mSettings.numLight = static_cast<uint32_t>(curLightCount);

            XMFLOAT3 lightColor = mLights[0].color;
            if (ImGui::ColorEdit3("Light Color", &(lightColor.x), ImGuiColorEditFlags_Float)) {
                for (uint32_t i = 0; i < LIGHT_COUNT; ++i) {
                    mLights[i].color = lightColor;
                }
                mDirtyFlag = true;
            }

            static int component = 0;
            ImGui::RadioButton("Disable", &component, 0);
            ImGui::SameLine();
            ImGui::RadioButton("F", &component, 1); 
            ImGui::SameLine();
            ImGui::RadioButton("D", &component, 2);
            ImGui::SameLine();
            ImGui::RadioButton("G", &component, 3);

            ImGui::Checkbox("Enable Texture", (bool *)&mAppSettings.enableTexture);
            ImGui::Checkbox("Enable IBL", (bool *)&mAppSettings.enableIBL);
            if (!mAppSettings.enableIBL) {
                ImGui::ColorEdit3("Ambient", &(mSettings.ambientColor.x));
            }

            uint32_t state = 0;
            switch (component) {
                case 1: {
                    if (mAppSettings.enableTexture) {
                        state = PbrPass::FactorFHasTex;
                    } else {
                        state = PbrPass::FactorFNoTex;
                    }
                } break;

                case 2: {
                    if (mAppSettings.enableTexture) {
                        state = PbrPass::FactorDHasTex;
                    } else {
                        state = PbrPass::FactorDNoTex;
                    }
                } break;

                case 3: {
                    if (mAppSettings.enableTexture) {
                        state = PbrPass::FactorGHasTex;
                    } else {
                        state = PbrPass::FactorGNoTex;
                    }
                } break;

                default: {
                    if (mAppSettings.enableTexture) {
                        state |= 0b01;
                    }
                    if (mAppSettings.enableIBL) {
                        state |= 0b10;
                    }
                } break;
            }
            mAppSettings.pbrPassState = state;

            if (!mAppSettings.enableTexture) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "Material");
                PbrDrawable *drawable = mDrawables[mDrawIndex];
                ImGui::ColorEdit3("Albdo", &(drawable->GetMaterial().albdo.x));
                ImGui::SliderFloat("Matelness", &(drawable->GetMaterial().metalness), 0.04f, 1.0f);
                ImGui::SliderFloat("Roughness", &(drawable->GetMaterial().roughness), 0.04f, 1.0f);
            }
        ImGui::EndChild();
    ImGui::End();

    if (mAppSettings.showEnvironmentImage) {
        ImGui::SetNextWindowSize(ImVec2(530.0f, 300.0f), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(360.0f, 5.0f), ImGuiCond_Once);
        ImGui::Begin("Environment Image", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::BeginChild("Environment Image");
                ImGui::Image(mEnvTexture, ImVec2(512.0f, 256.0f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            ImGui::EndChild();
        ImGui::End();
    }
    if (mAppSettings.showIrradianceImage) {
        ImGui::SetNextWindowSize(ImVec2(530.0f, 300.0f), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(360.0f, 310.0f), ImGuiCond_Once);
        ImGui::Begin("Irradiance Image", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::BeginChild("Irradiance Image");
                ImGui::Image(mIrrTexture, ImVec2(512.0f, 256.0f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            ImGui::EndChild();
        ImGui::End();
    }
    if (mAppSettings.showPrefilteredImage) {
        ImGui::SetNextWindowSize(ImVec2(300.0f, 190.0f), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(360.0f, 615.0f), ImGuiCond_Once);
        ImGui::Begin("Prefiltered Image", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::BeginChild("Prefiltered Image");
                const static char* mipItems[] = { "0", "1", "2", "3", "4", "5"};
                static int curItem = 0;
                ImGui::Combo("Mipmap Level", &curItem, mipItems, IM_ARRAYSIZE(mipItems));
                mGUI->SetImageMipLevel(mBlurredEnvTexture, curItem);
                float width = static_cast<float>(mBlurredEnvTexture->GetWidth() >> curItem);
                float height = static_cast<float>(mBlurredEnvTexture->GetHeight() >> curItem);
                ImGui::Image(mBlurredEnvTexture, ImVec2(width, height), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            ImGui::EndChild();
        ImGui::End();
    }
    if (mAppSettings.showBRDFLookupImage) {
        ImGui::SetNextWindowSize(ImVec2(530.0f, 550.0f), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(900.0f, 5.0f), ImGuiCond_Once);
        ImGui::Begin("BRDF Lookup Image", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::BeginChild("BRDF Lookup Image");
                float width = static_cast<float>(mBRDFLookupTexture->GetWidth());
                float height = static_cast<float>(mBRDFLookupTexture->GetHeight());
                ImGui::Image(mBRDFLookupTexture, ImVec2(width, height), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            ImGui::EndChild();
        ImGui::End();
    }

    mGUI->EndFrame(mCurrentFrame);
}

void PbrExample::Render(void) {
    Render::gCommand->Begin();

    // TODO: GPU resource sync
    if (mDirtyFlag) {
        Render::gCommand->UploadBuffer(mLightsBuffer, 0, mLights, mLightsBuffer->GetBufferSize());
        mDirtyFlag = false;
    }

    Render::gCommand->SetViewportAndScissor(0, 0, mWidth, mHeight);

    Render::RenderTargetBuffer **renderTargets = (Render::gMSAAState == Render::DisableMSAA ? Render::gRenderTarget : Render::gRenderTargetMSAA);

    Render::gCommand->TransitResource(renderTargets[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);
    Render::gCommand->SetRenderTarget(renderTargets[mCurrentFrame], Render::gDepthStencil);

    Render::gCommand->ClearColor(renderTargets[mCurrentFrame]);
    Render::gCommand->ClearDepth(Render::gDepthStencil);

    mPbrPass->PreviousRender((PbrPass::State)mAppSettings.pbrPassState);
    mPbrPass->Render(mCurrentFrame, mDrawables[mDrawIndex]);
    if (mAppSettings.enableSkybox) {
        mSkyboxPass->Render(mCurrentFrame, mTextureHeap, 0);
    }
    mGUI->Draw(mCurrentFrame, renderTargets[mCurrentFrame]);

    if (Render::gMSAAState != Render::DisableMSAA) {
        Render::gCommand->TransitResource(Render::gRenderTargetMSAA[mCurrentFrame], D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RESOLVE_DEST);
        Render::gCommand->ResolveSubresource(Render::gRenderTarget[mCurrentFrame], Render::gRenderTargetMSAA[mCurrentFrame], Render::gRenderTargetFormat);
    }

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);

    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}
