#include "pch.h"
#include "DXRExample.h"
#include <sys/timeb.h>

const static wchar_t *RayGenerationName = L"RayGener";

const static wchar_t *PrimaryMissName = L"PrimaryMiss";
const static wchar_t *PrimaryHitGroupName = L"PrimaryHitGroup";
const static wchar_t *PrimaryAnyHitName = L"PrimaryAnyHit";
const static wchar_t *PrimaryClosetHitName = L"PrimaryClosestHit";

const static wchar_t *IndirectMissName = L"IndirectMiss";
const static wchar_t *IndirectHitGroupName = L"IndirectHitGroup";
const static wchar_t *IndirectAnyHitName = L"IndirectAnyHit";
const static wchar_t *IndirectClosetHitName = L"IndirectClosestHit";

const static wchar_t *ShadowMissName = L"ShadowMiss";
const static wchar_t *ShadowHitGroupName = L"ShadowHitGroup";
const static wchar_t *ShadowAnyHitName = L"ShadowAnyHit";
const static wchar_t *ShadowClosetHitName = L"ShadowClosestHit";

DXRExample::DXRExample(HWND hwnd)
: Example(hwnd)
, mCamera(nullptr)
, mSpeedX(0.0f)
, mSpeedZ(0.0f)
, mIsRotating(false)
, mLastMousePos(0)
, mCurrentMousePos(0)
, mFrameCount(0)
, mAccumCount(0)
, mMaxRayDepth(3)
, mCurrentFrame(0)
, mGlobalRootSignature(nullptr)
, mLocalRootSignature(nullptr)
, mRayTracingState(nullptr)
, mDescriptorHeap(nullptr)
, mVertices(nullptr)
, mIndices(nullptr)
, mGeometries(nullptr)
, mLights(nullptr)
, mRaytracingOutput(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
, mEnvTexture(nullptr)
, mSettingsCB(nullptr)
, mSceneCB(nullptr)
, mCameraCB(nullptr)
, mScene(nullptr)
, mTLAS(nullptr)
, mSPRootSignature(nullptr)
, mSPGraphicsState(nullptr)
, mSPVertexBuffer(nullptr)
{
    for (auto &fence : mFenceValues) {
        fence = 1;
    }

    WINDOWINFO windowInfo;
    GetWindowInfo(mHwnd, &windowInfo);
    mWidth = windowInfo.rcClient.right - windowInfo.rcClient.left;
    mHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;

    struct timeb tb;
    ftime(&tb);
    mRang = std::mt19937( uint32_t(tb.time * 1000 + tb.millitm) );
}

DXRExample::~DXRExample(void) {
    DeleteAndSetNull(mCamera);
}

void DXRExample::Init(void) {
    Render::Initialize(mHwnd);
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    InitScene();
    CreateRootSignature();
    CreateRayTracingPipelineState();
    BuildGeometry();
    BuildAccelerationStructure();
    BuildShaderTables();
    CreateRaytracingOutput();
    PrepareScreenPass();

    mTimer.Reset();
}

void DXRExample::Update(void) {
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
        mCamera->MoveForward(deltaSecond * mSpeedZ * 200);
    }
    if (mSpeedX != 0.0f) {
        mCamera->MoveRight(deltaSecond * mSpeedX * 200);
    }

    float jitterX = mRangDist(mRang) - 0.5f;
    float jitterY = mRangDist(mRang) - 0.5f;
    if (mIsRotating || mSpeedZ != 0.0f || mSpeedX != 0.0f) {
        mAccumCount = 0;
        jitterX = 0;
        jitterY = 0;
    }

    mCamera->UpdateMatrixs();

    mSettings.enableAccumulate = 1;
    mSettings.enableJitterCamera = 0;
    mSettings.enableLensCamera = 0;
    mSettings.enableEnvironmentMap = 0;
    mSettings.enableIndirectLight = 1;

    XMStoreFloat4(&mCameraConsts.position, mCamera->GetPosition());
    XMStoreFloat4(&mCameraConsts.u, mCamera->GetU());
    XMStoreFloat4(&mCameraConsts.v, mCamera->GetV());
    XMStoreFloat4(&mCameraConsts.w, mCamera->GetW());
    mCameraConsts.jitter = { jitterX, jitterY };
    mCameraConsts.lensRadius = mCamera->GetLensRadius();
    mCameraConsts.focalLength = mCamera->GetFocalLength();

    mSceneConsts.bgColor = { 0.5f, 0.7f, 1.0f, 1.0f };
    mSceneConsts.lightCount = 1;
    mSceneConsts.frameCount = mFrameCount ++;
    mSceneConsts.accumCount = mAccumCount ++;
    mSceneConsts.maxPayDepth = mMaxRayDepth;
}

void DXRExample::Render(void) {
    Render::gCommand->Begin();

    Render::gCommand->SetRootSignature(mGlobalRootSignature);

    // Copy the updated scene constant buffer to GPU.
    mSettingsCB->CopyData(&mSettings, sizeof(AppSettings), 0, mCurrentFrame);
    mSceneCB->CopyData(&mSceneConsts, sizeof(SceneConstants), 0, mCurrentFrame);
    mCameraCB->CopyData(&mCameraConsts, sizeof(CameraConstants), 0, mCurrentFrame);

    Render::gCommand->SetComputeRootConstantBufferView(GlobalRootSignatureParams::AppSettingsSlot, mSettingsCB->GetGPUAddress(0, mCurrentFrame));
    Render::gCommand->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantsSlot, mSceneCB->GetGPUAddress(0, mCurrentFrame));
    Render::gCommand->SetComputeRootConstantBufferView(GlobalRootSignatureParams::CameraConstantsSlot, mCameraCB->GetGPUAddress(0, mCurrentFrame));


    // Bind the heaps, acceleration structure and dispatch rays.
    Render::DescriptorHeap *heaps[] = { mDescriptorHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    // Set index and successive vertex buffer decriptor tables
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::BuffersSlot, mIndices->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::EnvTexturesSlot, mEnvTexture->GetSRVHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::TexturesSlot, mTextures[0]->GetSRVHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::SamplerSlot, mSampler->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, mRaytracingOutput->GetUAVHandle());
    Render::gCommand->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, mTLAS->GetResult());

    Render::gCommand->SetRayTracingState(mRayTracingState);
    Render::gCommand->DispatchRay(mRayTracingState, mWidth, mHeight);

    // screen pass
    Render::gCommand->SetPipelineState(mSPGraphicsState);
    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);
    Render::gCommand->SetViewportAndScissor(0, 0, mWidth, mHeight);
    Render::gCommand->SetRenderTarget(Render::gRenderTarget[mCurrentFrame], nullptr);
    Render::gCommand->SetRootSignature(mSPRootSignature);
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    Render::gCommand->SetGraphicsRootDescriptorTable(SPRootSignatureParams::SPTextureSlot, mRaytracingOutput->GetSRVHandle());
    Render::gCommand->SetGraphicsRootDescriptorTable(SPRootSignatureParams::SPSamplerSlot, mSampler->GetHandle());
    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVertices(mSPVertexBufferView);
    Render::gCommand->DrawInstanced(6);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);


    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}

void DXRExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    for (auto blas : mBLASes) { delete blas; }
    mBLASes.clear();
    for (auto texture : mTextures) { delete texture; }
    mTextures.clear();

    DeleteAndSetNull(mSPVertexBuffer);
    DeleteAndSetNull(mSPRootSignature);
    DeleteAndSetNull(mSPGraphicsState);
    DeleteAndSetNull(mEnvTexture);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mScene);
    DeleteAndSetNull(mTLAS);
    DeleteAndSetNull(mRaytracingOutput);
    DeleteAndSetNull(mSettingsCB);
    DeleteAndSetNull(mSceneCB);
    DeleteAndSetNull(mCameraCB);
    DeleteAndSetNull(mIndices);
    DeleteAndSetNull(mVertices);
    DeleteAndSetNull(mGeometries);
    DeleteAndSetNull(mLights);
    DeleteAndSetNull(mDescriptorHeap);
    DeleteAndSetNull(mRayTracingState);
    DeleteAndSetNull(mLocalRootSignature);
    DeleteAndSetNull(mGlobalRootSignature);

    Render::Terminate();
}

// Windows virtual key code: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
void DXRExample::OnKeyDown(uint8_t key) {
    switch (key) {
        // W key
        case 0x57: mSpeedZ = 1.0f; break;
        // S key
        case 0x53: mSpeedZ = -1.0f; break;
        // A key
        case 0x41: mSpeedX = -1.0f; break;
        // D key
        case 0x44: mSpeedX = 1.0f; break;
        default: break;
    }
}

void DXRExample::OnKeyUp(uint8_t key) {
    switch (key) {
        case 0x57: // W key
        case 0x53: // S key
            mSpeedZ = 0.0f; 
            break;
        case 0x41: // A key
        case 0x44: // D key
            mSpeedX = 0.0f; 
            break;
        default: break;
    }
}

void DXRExample::OnMouseLButtonDown(int64_t pos) {
    mIsRotating = true;
    mLastMousePos = pos;
    mCurrentMousePos = pos;
}

void DXRExample::OnMouseLButtonUp(int64_t pos) {
    mIsRotating = false;
}

void DXRExample::OnMouseMove(int64_t pos) {
    mCurrentMousePos = pos;
}

void DXRExample::InitScene(void) {
    mScene = Utils::Model::LoadFromFile("..\\..\\Models\\sponza\\sponza.obj");
    assert(mScene);

    mCamera = new Utils::Camera(XM_PIDIV4, (float)mWidth / (float)mHeight, 0.1f, 1000.0f, 
                                XMFLOAT4(1098.72424f, 651.495361f, -38.6905518f, 0.0f),
                                XMFLOAT4(0.0f, 651.495361f, 0.0f, 0.0f));
    mCamera->SetLensParams(32.0f, 2.0f);

    mSettingsCB = new Render::ConstantBuffer(sizeof(AppSettings), 1);
    mSceneCB = new Render::ConstantBuffer(sizeof(SceneConstants), 1);
    mCameraCB = new Render::ConstantBuffer(sizeof(CameraConstants), 1);
    // 4: vertexBuf, indexBuf, GeometryBuf, lightBuf
    // 2: rtOutput SRV + UAV
    // 1: evtTex
    mDescriptorHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(4 + 2 + 1 + mScene->mImages.size()));
    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
    mSampler = new Render::Sampler();
    mSampler->Create(mSamplerHeap->Allocate());

    Utils::Image *envImage = Utils::Image::CreateFromFile("..\\..\\Models\\dirt_road.hdr");

    Render::gCommand->Begin();

    mEnvTexture = new Render::PixelBuffer(envImage->GetPitch(), envImage->GetWidth(), envImage->GetHeight(), envImage->GetMipLevels(), envImage->GetDXGIFormat());
    Render::gCommand->UploadTexture(mEnvTexture, envImage->GetPixels());
    mEnvTexture->CreateSRV(mDescriptorHeap->Allocate());

    mTextures.reserve(mScene->mImages.size());
    for (auto image : mScene->mImages) {
        Render::PixelBuffer *texture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), image->GetMipLevels(), image->GetDXGIFormat());
        std::vector<D3D12_SUBRESOURCE_DATA> subResources;
        image->GetSubResources(subResources);
        Render::gCommand->UploadTexture(texture, subResources.data(), static_cast<uint32_t>(subResources.size()));
        texture->CreateSRV(mDescriptorHeap->Allocate());
        mTextures.push_back(texture);
    }

    Render::gCommand->End(true);

    delete envImage;
}

void DXRExample::CreateRootSignature(void) {
    mGlobalRootSignature = new Render::RootSignature(Render::RootSignature::Compute, GlobalRootSignatureParams::SlotCount);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::AccelerationStructureSlot, D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::AppSettingsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 0);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::SceneConstantsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 1);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::CameraConstantsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 2);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::BuffersSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::EnvTexturesSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::TexturesSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, static_cast<uint32_t>(mScene->mImages.size()), 6);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::SamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mGlobalRootSignature->Create();

    mLocalRootSignature = new Render::RootSignature(Render::RootSignature::Compute, 0, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    mLocalRootSignature->Create();
}

void DXRExample::CreateRayTracingPipelineState(void) {

    mRayTracingState = new Render::RayTracingState(9);

    const wchar_t *shaderFuncs[] = {RayGenerationName, 
                                    PrimaryMissName, PrimaryAnyHitName, PrimaryClosetHitName,
                                    IndirectMissName, IndirectAnyHitName, IndirectClosetHitName,
                                    ShadowMissName, ShadowAnyHitName, ShadowClosetHitName};
    mRayTracingState->AddDXILLibrary("RtMain.cso", shaderFuncs, _countof(shaderFuncs));

    mRayTracingState->AddRayTracingShaderConfig(2 * sizeof(XMFLOAT4), sizeof(XMFLOAT2) /*float2 barycentrics*/);

    mRayTracingState->AddHitGroup(PrimaryHitGroupName, PrimaryClosetHitName, PrimaryAnyHitName);
    mRayTracingState->AddHitGroup(IndirectHitGroupName, IndirectClosetHitName, IndirectAnyHitName);
    mRayTracingState->AddHitGroup(ShadowHitGroupName, ShadowClosetHitName, ShadowAnyHitName);

    const wchar_t *hitGroups[] = { PrimaryHitGroupName, IndirectHitGroupName, ShadowHitGroupName };
    uint32_t lrsIdx = mRayTracingState->AddLocalRootSignature(mLocalRootSignature);
    mRayTracingState->AddSubObjectToExportsAssociation(lrsIdx, hitGroups, _countof(hitGroups));

    mRayTracingState->AddGlobalRootSignature(mGlobalRootSignature);

    mRayTracingState->AddRayTracingPipelineConfig(mMaxRayDepth);

    mRayTracingState->Create();
}

void DXRExample::BuildGeometry(void) {
    constexpr uint32_t lightCount = 1;

    mIndices = new Render::GPUBuffer(mScene->mIndices.size() * sizeof(uint32_t));
    mVertices = new Render::GPUBuffer(mScene->mVertices.size() * sizeof(Utils::Scene::Vertex));
    mGeometries = new Render::GPUBuffer(mScene->mShapes.size() * sizeof(Geometry));
    mLights = new Render::GPUBuffer(lightCount * sizeof(Light));

    Geometry *geometries = new Geometry[mScene->mShapes.size()];
    for (uint32_t i = 0; i < mScene->mShapes.size(); ++i) {
        auto &shape = mScene->mShapes[i];
        ASSERT_PRINT(shape.diffuseTex != ~0 && shape.specularTex != ~0);
        geometries[i].indexInfo = { shape.indexOffset, shape.indexCount, 0, 0 };
        geometries[i].texInfo = { shape.diffuseTex, shape.ambientTex, shape.specularTex, shape.normalTex };
    }

    Light lights[] = {
        { DirectLight, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.1f}, {2.0f, 2.0f, 2.0f} },
        //{ PointLight, XM_2PI, 0.0f, -1.0f, {-4.645481586456299f, 1.5427508354187012f, -1.488459825515747f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f} },
        //{ PointLight, XM_2PI, 0.0f, -1.0f, {-1.016136884689331f, 1.4740270376205445f, -1.4256235361099244f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f} },
    };


    Render::gCommand->Begin();

    Render::gCommand->UploadBuffer(mIndices, 0, mScene->mIndices.data(), mIndices->GetBufferSize());
    Render::gCommand->UploadBuffer(mVertices, 0, mScene->mVertices.data(), mVertices->GetBufferSize());
    Render::gCommand->UploadBuffer(mGeometries, 0, geometries, mGeometries->GetBufferSize());
    Render::gCommand->UploadBuffer(mLights, 0, lights, mLights->GetBufferSize());

    mIndices->CreateIndexBufferSRV(mDescriptorHeap->Allocate(), static_cast<uint32_t>(mScene->mIndices.size()));
    mVertices->CreateStructBufferSRV(mDescriptorHeap->Allocate(), static_cast<uint32_t>(mScene->mVertices.size()), sizeof(Utils::Scene::Vertex));
    mGeometries->CreateStructBufferSRV(mDescriptorHeap->Allocate(), static_cast<uint32_t>(mScene->mShapes.size()), sizeof(Geometry));
    mLights->CreateStructBufferSRV(mDescriptorHeap->Allocate(), lightCount, sizeof(Light));

    Render::gCommand->End(true);

    delete [] geometries;
}

void DXRExample::BuildAccelerationStructure(void) {
    mTLAS = new Render::TopLevelAccelerationStructure();
    XMMATRIX transform = XMMatrixIdentity();

    uint32_t shapeCount = static_cast<uint32_t>(mScene->mShapes.size());
    mBLASes.reserve(shapeCount);
    for (uint32_t i = 0; i < shapeCount; ++i) {
        auto &shape = mScene->mShapes[i];
        BLAS *blas = new BLAS();
        blas->AddTriangles(mIndices, shape.indexOffset, shape.indexCount, false, mVertices, 0, static_cast<uint32_t>(mScene->mVertices.size()), sizeof(Utils::Scene::Vertex));
        blas->PreBuild();
        mBLASes.push_back(blas);
        mTLAS->AddInstance(blas, i, 0, transform);
    }

    mTLAS->PreBuild();

    Render::gCommand->Begin();
    for (auto blas : mBLASes) {
        Render::gCommand->BuildAccelerationStructure(blas);
    }
    Render::gCommand->BuildAccelerationStructure(mTLAS);
    Render::gCommand->End(true);

    for (auto blas : mBLASes) {
        blas->PostBuild();
    }
    mTLAS->PostBuild();
}

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void DXRExample::BuildShaderTables(void) {
    const wchar_t *rayGens[] = { RayGenerationName };
    const wchar_t *misses[] = { PrimaryMissName, IndirectMissName, ShadowMissName };
    const wchar_t *hipGroups[] = { PrimaryHitGroupName, IndirectHitGroupName, ShadowHitGroupName };
    mRayTracingState->BuildShaderTable(rayGens, _countof(rayGens), misses, _countof(misses), hipGroups, _countof(hipGroups));
}

// Create 2D output texture for raytracing.
void DXRExample::CreateRaytracingOutput(void) {
    DXGI_FORMAT format = Render::gRenderTarget[0]->GetFormat();
    uint32_t width = Render::gRenderTarget[0]->GetWidth();
    uint32_t height = Render::gRenderTarget[0]->GetHeight();

    // Create the output resource. The dimensions and format should match the swap-chain.
    mRaytracingOutput = new Render::PixelBuffer(width * Render::BytesPerPixel(DXGI_FORMAT_R32G32B32A32_FLOAT), width, height, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mRaytracingOutput->CreateUAV(mDescriptorHeap->Allocate());
    mRaytracingOutput->CreateSRV(mDescriptorHeap->Allocate());
}

void DXRExample::PrepareScreenPass(void) {
    mSPRootSignature = new Render::RootSignature(Render::RootSignature::Graphics, SPRootSignatureParams::SPSlotCount, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mSPRootSignature->SetDescriptorTable(SPRootSignatureParams::SPTextureSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    mSPRootSignature->SetDescriptorTable(SPRootSignatureParams::SPSamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mSPRootSignature->Create();

    // input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    mSPGraphicsState = new Render::GraphicsState();
    mSPGraphicsState->GetInputLayout() = { inputElementDesc, _countof(inputElementDesc) };
    mSPGraphicsState->EnableDepth(false);
    mSPGraphicsState->LoadVertexShader("Display.vs.cso");
    mSPGraphicsState->LoadPixelShader("Display.ps.cso");
    mSPGraphicsState->Create(mSPRootSignature);

    float vertices[] = {
        // triangle 1
        -1.0f, 1.0f, 0.0f, 0.0f,
         1.0f, 1.0f, 1.0f, 0.0f,
         1.0f,-1.0f, 1.0f, 1.0f,
        // triangle 2
        -1.0f, 1.0f, 0.0f, 0.0f,
         1.0f,-1.0f, 1.0f, 1.0f,
        -1.0f,-1.0f, 0.0f, 1.0f
    };

    mSPVertexBuffer = new Render::GPUBuffer(sizeof(vertices));

    Render::gCommand->Begin();
    Render::gCommand->UploadBuffer(mSPVertexBuffer, 0, vertices, sizeof(vertices));
    Render::gCommand->End(true);

    mSPVertexBufferView = mSPVertexBuffer->FillVertexBufferView(0, sizeof(vertices), 16);
}

