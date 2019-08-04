#include "pch.h"
#include "DXRExample.h"
#include <sys/timeb.h>

const static wchar_t *RayGenerationName = L"RayGener";

const static wchar_t *PrimaryMissName = L"PrimaryMiss";
const static wchar_t *PrimaryHitGroupName = L"PrimaryHitGroup";
const static wchar_t *PrimaryAnyHitName = L"PrimaryAnyHit";
const static wchar_t *PrimaryClosetHitName = L"PrimaryClosestHit";

const static wchar_t *AOMissName = L"AOMiss";
const static wchar_t *AOHitGroupName = L"AOHitGroup";
const static wchar_t *AOAnyHitName = L"AOAnyHit";
const static wchar_t *AOClosetHitName = L"AOClosestHit";

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
, mCurrentFrame(0)
, mGlobalRootSignature(nullptr)
, mLocalRootSignature(nullptr)
, mRayTracingState(nullptr)
, mDescriptorHeap(nullptr)
, mVertices(nullptr)
, mIndices(nullptr)
, mGeometries(nullptr)
, mRaytracingOutput(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
, mSettingsCB(nullptr)
, mSceneCB(nullptr)
, mCameraCB(nullptr)
, mMeshCB(nullptr)
, mScene(nullptr)
, mTLAS(nullptr)
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

    mTimer.Reset();
}

void DXRExample::Update(void) {
    static float elapse = 0.0f;
    static float maxAngle = 45.0f;
    static float factor = 10.0f;

    mTimer.Tick();
    float deltaSecond = static_cast<float>(mTimer.GetElapsedSeconds());

    elapse += deltaSecond * factor;
    if (elapse > maxAngle) {
        elapse = maxAngle;
        factor = -factor;
    } else if (elapse < -maxAngle) {
        elapse = -maxAngle;
        factor = -factor;
    }

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
        mCamera->MoveForward(deltaSecond * mSpeedZ * 2);
    }
    if (mSpeedX != 0.0f) {
        mCamera->MoveRight(deltaSecond * mSpeedX * 2);
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
    mSettings.enableJitterCamera = 1;
    mSettings.enableLensCamera = 1;

    XMStoreFloat4(&mCameraConsts.pos, mCamera->GetPosition());
    XMStoreFloat4(&mCameraConsts.u, mCamera->GetU());
    XMStoreFloat4(&mCameraConsts.v, mCamera->GetV());
    XMStoreFloat4(&mCameraConsts.w, mCamera->GetW());
    mCameraConsts.jitter = { jitterX, jitterY };
    mCameraConsts.lensRadius = mCamera->GetLensRadius();
    mCameraConsts.focalLength = mCamera->GetFocalLength();

    mSceneConsts.bgColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    mSceneConsts.frameCount = mFrameCount ++;
    mSceneConsts.accumCount = mAccumCount ++;
    mSceneConsts.aoRadius = 0.63f;
}

void DXRExample::Render(void) {
    Render::gCommand->Begin();

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);
    Render::gCommand->SetRootSignature(mGlobalRootSignature);

    // Copy the updated scene constant buffer to GPU.
    void *mappedConstantData = mSettingsCB->GetMappedBuffer(0, mCurrentFrame);
    memcpy(mappedConstantData, &mSettings, sizeof(AppSettings));
    mappedConstantData = mSceneCB->GetMappedBuffer(0, mCurrentFrame);
    memcpy(mappedConstantData, &mSceneConsts, sizeof(SceneConstants));
    mappedConstantData = mCameraCB->GetMappedBuffer(0, mCurrentFrame);
    memcpy(mappedConstantData, &mCameraConsts, sizeof(CameraConstants));

    Render::gCommand->SetComputeRootConstantBufferView(GlobalRootSignatureParams::AppSettingsSlot, mSettingsCB->GetGPUAddress(0, mCurrentFrame));
    Render::gCommand->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantsSlot, mSceneCB->GetGPUAddress(0, mCurrentFrame));
    Render::gCommand->SetComputeRootConstantBufferView(GlobalRootSignatureParams::CameraConstantsSlot, mCameraCB->GetGPUAddress(0, mCurrentFrame));


    // Bind the heaps, acceleration structure and dispatch rays.
    Render::DescriptorHeap *heaps[] = { mDescriptorHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    // Set index and successive vertex buffer decriptor tables
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, mIndices->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::TexturesSlot, mTextures.At(0)->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::SamplerSlot, mSampler->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, mRaytracingOutput->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputColorSlot, mDisplayColor->GetHandle());
    Render::gCommand->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, mTLAS->GetResult());

    Render::gCommand->SetRayTracingState(mRayTracingState);
    Render::gCommand->DispatchRay(mRayTracingState, mWidth, mHeight);

    Render::gCommand->CopyResource(Render::gRenderTarget[mCurrentFrame], mDisplayColor);
    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);
    Render::gCommand->TransitResource(mDisplayColor, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}

void DXRExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    for (uint32_t i = 0; i < mBLASes.Count(); ++i) { delete mBLASes.At(i); }
    mBLASes.Clear();
    for (uint32_t i = 0; i < mTextures.Count(); ++i) { delete mTextures.At(i); }
    mTextures.Clear();

    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mScene);
    DeleteAndSetNull(mTLAS);
    DeleteAndSetNull(mDisplayColor);
    DeleteAndSetNull(mRaytracingOutput);
    DeleteAndSetNull(mMeshCB);
    DeleteAndSetNull(mSettingsCB);
    DeleteAndSetNull(mSceneCB);
    DeleteAndSetNull(mCameraCB);
    DeleteAndSetNull(mIndices);
    DeleteAndSetNull(mVertices);
    DeleteAndSetNull(mGeometries);
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
    mScene = Utils::Model::LoadFromMMB("Models\\pink_room.mmb");
    assert(mScene);

    mCamera = new Utils::Camera(XM_PIDIV4, (float)mWidth / (float)mHeight, 0.1f, 100.0f, 
                                XMFLOAT4(-2.706775665283203f, 0.85294109582901f, -3.112438678741455f, 1.0f), 
                                XMFLOAT4(-2.347264528274536f, 0.7383297681808472f, -2.1863629817962648f, 1.0f), 
                                XMFLOAT4(0.038521841168403628f, 0.9933950304985046f, 0.1079813688993454f, 0.0f));
    mCamera->SetLensParams(32.0f, 2.0f);

    mSettingsCB = new Render::ConstantBuffer(sizeof(AppSettings), 1);
    mSceneCB = new Render::ConstantBuffer(sizeof(SceneConstants), 1);
    mCameraCB = new Render::ConstantBuffer(sizeof(CameraConstants), 1);
    mDescriptorHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3 + 2 + mScene->mImages.Count());
    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
    mSampler = new Render::Sampler();
    mSampler->Create(mSamplerHeap->Allocate());

    Render::gCommand->Begin();

    mTextures.Reserve(mScene->mImages.Count());
    for (uint32_t i = 0; i < mScene->mImages.Count(); ++i) {
        auto &image = mScene->mImages.At(i);
        Render::PixelBuffer *texture = new Render::PixelBuffer(image.width, image.width, image.height, DXGI_FORMAT_R8G8B8A8_UNORM);
        Render::gCommand->UploadTexture(texture, image.pixels);
        texture->CreateSRV(mDescriptorHeap->Allocate());
        mTextures.PushBack(texture);
    }

    Render::gCommand->End(true);
}

void DXRExample::CreateRootSignature(void) {
    mGlobalRootSignature = new Render::RootSignature(Render::RootSignature::Compute, GlobalRootSignatureParams::SlotCount);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::OutputColorSlot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::AccelerationStructureSlot, D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::AppSettingsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 0);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::SceneConstantsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 1);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::CameraConstantsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 2);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::TexturesSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mScene->mImages.Count(), 4);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::SamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mGlobalRootSignature->Create();

    mLocalRootSignature = new Render::RootSignature(Render::RootSignature::Compute, 0, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    mLocalRootSignature->Create();
}

void DXRExample::CreateRayTracingPipelineState(void) {

    mRayTracingState = new Render::RayTracingState(8);

    const wchar_t *shaderFuncs[] = { RayGenerationName, 
                                     PrimaryMissName, PrimaryAnyHitName, PrimaryClosetHitName,
                                     AOMissName, AOAnyHitName, AOClosetHitName};
    mRayTracingState->AddDXILLibrary("RayTracer.cso", shaderFuncs, _countof(shaderFuncs));

    mRayTracingState->AddRayTracingShaderConfig(2 * sizeof(XMFLOAT4), sizeof(XMFLOAT2) /*float2 barycentrics*/);

    mRayTracingState->AddHitGroup(PrimaryHitGroupName, PrimaryClosetHitName, PrimaryAnyHitName);
    mRayTracingState->AddHitGroup(AOHitGroupName, AOClosetHitName, AOAnyHitName);

    const wchar_t *hitGroups[] = { PrimaryHitGroupName, AOHitGroupName };
    uint32_t lrsIdx = mRayTracingState->AddLocalRootSignature(mLocalRootSignature);
    mRayTracingState->AddSubObjectToExportsAssociation(lrsIdx, hitGroups, _countof(hitGroups));

    mRayTracingState->AddGlobalRootSignature(mGlobalRootSignature);

    mRayTracingState->AddRayTracingPipelineConfig(2);

    mRayTracingState->Create();
}

void DXRExample::BuildGeometry(void) {
    mIndices = new Render::GPUBuffer(mScene->mIndices.Count() * sizeof(uint32_t));
    mVertices = new Render::GPUBuffer(mScene->mVertices.Count() * sizeof(Utils::Scene::Vertex));
    mGeometries = new Render::GPUBuffer(mScene->mShapes.Count() * sizeof(Geometry));
    Geometry *geometries = new Geometry[mScene->mShapes.Count()];
    for (uint32_t i = 0; i < mScene->mShapes.Count(); ++i) {
        auto &shape = mScene->mShapes.At(i);
        ASSERT_PRINT(shape.diffuseTex != ~0 && shape.specularTex != ~0);
        geometries[i].indexInfo = { shape.indexOffset, shape.indexCount, 0, 0 };
        geometries[i].texInfo = { shape.diffuseTex, shape.specularTex, shape.normalTex, 0 };
    }

    Render::gCommand->Begin();

    Render::gCommand->UploadBuffer(mIndices, 0, mScene->mIndices.Data(), mIndices->GetBufferSize());
    Render::gCommand->UploadBuffer(mVertices, 0, mScene->mVertices.Data(), mVertices->GetBufferSize());
    Render::gCommand->UploadBuffer(mGeometries, 0, geometries, mGeometries->GetBufferSize());

    mIndices->CreateIndexBufferSRV(mDescriptorHeap->Allocate(), mScene->mIndices.Count());
    mVertices->CreateStructBufferSRV(mDescriptorHeap->Allocate(), mScene->mVertices.Count(), sizeof(Utils::Scene::Vertex));
    mGeometries->CreateStructBufferSRV(mDescriptorHeap->Allocate(), mScene->mShapes.Count(), sizeof(Geometry));

    Render::gCommand->End(true);

    delete [] geometries;
}

void DXRExample::BuildAccelerationStructure(void) {
    mTLAS = new Render::TopLevelAccelerationStructure();
    XMMATRIX transform = XMMatrixIdentity();

    uint32_t shapeCount = mScene->mShapes.Count();
    mBLASes.Reserve(shapeCount);
    for (uint32_t i = 0; i < shapeCount; ++i) {
        auto &shape = mScene->mShapes.At(i);
        BLAS *blas = new BLAS();
        blas->AddTriangles(mIndices, shape.indexOffset, shape.indexCount, false, mVertices, 0, mScene->mVertices.Count(), sizeof(Utils::Scene::Vertex));
        blas->PreBuild();
        mBLASes.PushBack(blas);
        mTLAS->AddInstance(blas, i, 0, transform);
    }

    mTLAS->PreBuild();

    Render::gCommand->Begin();
    for (uint32_t i = 0; i < shapeCount; ++i) {
        Render::gCommand->BuildAccelerationStructure(mBLASes.At(i));
    }
    Render::gCommand->BuildAccelerationStructure(mTLAS);
    Render::gCommand->End(true);

    for (uint32_t i = 0; i < shapeCount; ++i) {
        mBLASes.At(i)->PostBuild();
    }
    mTLAS->PostBuild();
}

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void DXRExample::BuildShaderTables(void) {
    const wchar_t *rayGens[] = { RayGenerationName };
    const wchar_t *misses[] = { PrimaryMissName, AOMissName };
    const wchar_t *hipGroups[] = { PrimaryHitGroupName, AOHitGroupName };
    mRayTracingState->BuildShaderTable(rayGens, _countof(rayGens), misses, _countof(misses), hipGroups, _countof(hipGroups));
}

// Create 2D output texture for raytracing.
void DXRExample::CreateRaytracingOutput(void) {
    DXGI_FORMAT format = Render::gRenderTarget[0]->GetFormat();
    uint32_t width = Render::gRenderTarget[0]->GetWidth();
    uint32_t height = Render::gRenderTarget[0]->GetHeight();

    // Create the output resource. The dimensions and format should match the swap-chain.
    mRaytracingOutput = new Render::PixelBuffer(width, width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mRaytracingOutput->CreateUAV(mDescriptorHeap->Allocate());

    mDisplayColor = new Render::PixelBuffer(width, width, height, format, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mDisplayColor->CreateUAV(mDescriptorHeap->Allocate());
}

