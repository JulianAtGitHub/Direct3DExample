#include "pch.h"
#include "DXRExample.h"
#include "Core/Render/RenderCore.h"
#include "Core/Render/CommandContext.h"
#include "Core/Render/RootSignature.h"
#include "Core/Render/DescriptorHeap.h"
#include "Core/Render/RayTracingState.h"
#include "Core/Render/AccelerationStructure.h"
#include "Core/Render/Resource/GPUBuffer.h"
#include "Core/Render/Resource/UploadBuffer.h"
#include "Core/Render/Resource/ConstantBuffer.h"
#include "Core/Render/Resource/PixelBuffer.h"
#include "Core/Render/Resource/RenderTargetBuffer.h"

const static wchar_t *HitCubeGroupName = L"MyHitGroup";
const static wchar_t *HitShadowGroupName = L"MyHitShadowGroup";
const static wchar_t *RaygenShaderName = L"MyRaygenShader";
const static wchar_t *ClosestHitShaderName = L"MyClosestHitShader";
const static wchar_t *MissShaderName = L"MyMissShader";
const static wchar_t *MissShadowName = L"MyMissShadow";

static XMFLOAT4 cameraPosition(0.0f, 10.0f, 10.0f, 1.0f);
static XMFLOAT4 lightDirection(0.0f, -1.0f, 0.0f, 0.0f);
static XMFLOAT4 lightAmbientColor(0.1f, 0.1f, 0.1f, 1.0f);
static XMFLOAT4 lightDiffuseColor(0.7f, 0.7f, 0.7f, 1.0f);

namespace GlobalRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        SceneConstantSlot,
        VertexBuffersSlot,
        Count
    };
}

namespace LocalRootSignatureParams {
    enum Value {
        MeshConstantSlot = 0,
        Count
    };
}

DXRExample::DXRExample(HWND hwnd)
: Example(hwnd)
, mCurrentFrame(0)
, mGlobalRootSignature(nullptr)
, mLocalRootSignature(nullptr)
, mEmptyRootSignature(nullptr)
, mRayTracingState(nullptr)
, mDescriptorHeap(nullptr)
, mVertices(nullptr)
, mIndices(nullptr)
, mSceneConstantBuffer(nullptr)
, mRaytracingOutput(nullptr)
, mShaderTable(nullptr)
, mBLASCube(nullptr)
, mBLASPlane(nullptr)
, mTLAS(nullptr)
{
    for (auto &fence : mFenceValues) {
        fence = 1;
    }

    WINDOWINFO windowInfo;
    GetWindowInfo(mHwnd, &windowInfo);
    mWidth = windowInfo.rcClient.right - windowInfo.rcClient.left;
    mHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;
}

DXRExample::~DXRExample(void) {

}

void DXRExample::Init(void) {
    Render::Initialize(mHwnd);
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    InitScene();
    CreateRootSignature();
    CreateRayTracingPipelineState();
    CreateDescriptorHeap();
    BuildGeometry();
    BuildAccelerationStructure();
    CreateConstantBuffer();
    BuildShaderTables();
    CreateRaytracingOutput();

    UpdateCameraMatrices();
}

void DXRExample::Update(void) {
    static float elapse = 0.0f;
    static float factor = 0.2f;
    static float maxAngle = 45.0f;
    elapse += factor;
    if (elapse > maxAngle) {
        elapse = maxAngle;
        factor = -factor;
    } else if (elapse < -maxAngle) {
        elapse = -maxAngle;
        factor = -factor;
    }

    XMMATRIX rotate = XMMatrixRotationZ(XMConvertToRadians(elapse));
    mSceneConstBuf[mCurrentFrame].lightDirection = XMVector4Transform(XMLoadFloat4(&lightDirection), rotate);
}

void DXRExample::Render(void) {
    Render::gCommand->Begin();

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);
    Render::gCommand->SetComputeRootSignature(mGlobalRootSignature);

    // Copy the updated scene constant buffer to GPU.
    void *mappedConstantData = mSceneConstantBuffer->GetMappedBuffer(0, mCurrentFrame);
    memcpy(mappedConstantData, mSceneConstBuf + mCurrentFrame, sizeof(SceneConstantBuffer));
    Render::gCommand->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantSlot, mSceneConstantBuffer->GetGPUAddress(0, mCurrentFrame));

    // Bind the heaps, acceleration structure and dispatch rays.
    Render::DescriptorHeap *heaps[] = { mDescriptorHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, 1);
    // Set index and successive vertex buffer decriptor tables
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, mIndices->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, mRaytracingOutput->GetHandle());
    Render::gCommand->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, mTLAS->GetResult());

    Render::gCommand->SetRayTracingState(mRayTracingState);
    Render::gCommand->GetDXRCommandList()->DispatchRays(&mRaysDesc);

    Render::gCommand->CopyResource(Render::gRenderTarget[mCurrentFrame], mRaytracingOutput);
    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);
    Render::gCommand->TransitResource(mRaytracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}

void DXRExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    DeleteAndSetNull(mBLASCube);
    DeleteAndSetNull(mBLASPlane);
    DeleteAndSetNull(mTLAS);
    DeleteAndSetNull(mRaytracingOutput);
    DeleteAndSetNull(mShaderTable);
    DeleteAndSetNull(mSceneConstantBuffer);
    DeleteAndSetNull(mIndices);
    DeleteAndSetNull(mVertices);
    DeleteAndSetNull(mDescriptorHeap);
    DeleteAndSetNull(mRayTracingState);
    DeleteAndSetNull(mEmptyRootSignature);
    DeleteAndSetNull(mLocalRootSignature);
    DeleteAndSetNull(mGlobalRootSignature);

    Render::Terminate();
}

void DXRExample::InitScene(void) {
    mMeshConstBuf.albedo[0] = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    mMeshConstBuf.albedo[1] = XMFLOAT4(0.7f, 0.5f, 0.1f, 1.0f);
    mMeshConstBuf.offset.x = 0;
    mMeshConstBuf.offset.y = 12;
    for (auto &sceneConstBuf : mSceneConstBuf) {
        sceneConstBuf.cameraPosition = XMLoadFloat4(&cameraPosition);
        sceneConstBuf.lightDirection = XMLoadFloat4(&lightDirection);
        sceneConstBuf.lightAmbientColor = XMLoadFloat4(&lightAmbientColor);
        sceneConstBuf.lightDiffuseColor = XMLoadFloat4(&lightDiffuseColor);
    }
}

void DXRExample::CreateRootSignature(void) {
    mGlobalRootSignature = new Render::RootSignature(GlobalRootSignatureParams::Count);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::AccelerationStructureSlot, D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
    mGlobalRootSignature->SetDescriptor(GlobalRootSignatureParams::SceneConstantSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 0);
    mGlobalRootSignature->SetDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);
    mGlobalRootSignature->Create();

    mLocalRootSignature = new Render::RootSignature(LocalRootSignatureParams::Count, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    mLocalRootSignature->SetConstants(LocalRootSignatureParams::MeshConstantSlot, (uint32_t)(AlignUp(sizeof(mMeshConstBuf), 4) >> 2), 1);
    mLocalRootSignature->Create();

    mEmptyRootSignature = new Render::RootSignature(0, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    mEmptyRootSignature->Create();
}

// Create a raytracing pipeline state object (RTPSO).
// An RTPSO represents a full set of shaders reachable by a DispatchRays() call,
// with all configuration options resolved, such as local signatures and other state.
void DXRExample::CreateRayTracingPipelineState(void) {
    // Create 7 subobjects that combine into a RTPSO:
    // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
    // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
    // This simple sample utilizes default shader association except for local root signature subobject
    // which has an explicit association specified purely for demonstration purposes.
    // 1 - DXIL library
    // 1 - Shader config
    // 2 - Triangle hit group
    // 2 - Export Association
    // 2 - Local root signature and association
    // 1 - Global root signature
    // 1 - Pipeline config
    mRayTracingState = new Render::RayTracingState(10);

    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.

    // Define which shader exports to surface from the library.
    // If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
    // In this sample, this could be ommited for convenience since the sample uses all shaders in the library.
    constexpr uint32_t shaderCount = 4;
    const wchar_t *shaderFuncs[shaderCount] = { RaygenShaderName, ClosestHitShaderName, MissShaderName, MissShadowName };
    mRayTracingState->AddDXILLibrary("raytracing.cso", shaderFuncs, shaderCount);

    // Shader config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    mRayTracingState->AddRayTracingShaderConfig(sizeof(XMFLOAT4) /*float4 pixelColor*/, sizeof(XMFLOAT2) /*float2 barycentrics*/);

    // Triangle hit group
    // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
    // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.

    // Cube & Plane
    mRayTracingState->AddHitGroup(HitCubeGroupName, ClosestHitShaderName);

    // Local root signature and shader association
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    uint32_t lrsIdx1 = mRayTracingState->AddLocalRootSignature(mLocalRootSignature);
    mRayTracingState->AddSubObjectToExportsAssociation(lrsIdx1, &HitCubeGroupName, 1);

    // Shadow
    mRayTracingState->AddHitGroup(HitShadowGroupName);
    uint32_t lrsIdx2 = mRayTracingState->AddLocalRootSignature(mEmptyRootSignature);
    mRayTracingState->AddSubObjectToExportsAssociation(lrsIdx2, &HitShadowGroupName, 1);

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    mRayTracingState->AddGlobalRootSignature(mGlobalRootSignature);

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    // PERFOMANCE TIP: Set max recursion depth as low as needed 
    // as drivers may apply optimization strategies for low recursion depths.
    mRayTracingState->AddRayTracingPipelineConfig(2);

    mRayTracingState->Create();
}

void DXRExample::CreateDescriptorHeap(void) {
    // Allocate a heap for 5 descriptors:
    // 2 - vertex and index buffer SRVs
    // 1 - raytracing output texture SRV
    mDescriptorHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 5);
}

void DXRExample::BuildGeometry(void) {
    Render::gCommand->Begin();

    uint32_t indices[] = {
        // Cube
        0,1,3,
        3,1,2,

        5,4,6,
        6,4,7,

        8,9,11,
        11,9,10,

        13,12,14,
        14,12,15,

        16,17,19,
        19,17,18,

        21,20,22,
        22,20,23,

        // Plane
        24,25,27,
        27,25,26,
    };

    // Vertices positions and corresponding triangle normals.
    Vertex vertices[] = {
        // Cube
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },

        // Plane
        { XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
    };

    mIndices = new Render::GPUBuffer(sizeof(indices));
    mVertices = new Render::GPUBuffer(sizeof(vertices));

    Render::gCommand->UploadBuffer(mIndices, 0, indices, sizeof(indices));
    Render::gCommand->UploadBuffer(mVertices, 0, vertices, sizeof(vertices));

    mIndices->CreateIndexBufferSRV(mDescriptorHeap->Allocate(), ARRAYSIZE(indices));
    mVertices->CreateVertexBufferSRV(mDescriptorHeap->Allocate(), ARRAYSIZE(vertices), sizeof(Vertex));

    Render::gCommand->End(true);
}

void DXRExample::BuildAccelerationStructure(void) {
    constexpr uint32_t cubeIndexCount = 3 * 12;
    constexpr uint32_t planeIndexCount = 3 * 2;
    constexpr uint32_t vertexCount = 4 * 6 + 4 * 1;

    mBLASCube = new Render::BottomLevelAccelerationStructure();
    mBLASPlane = new Render::BottomLevelAccelerationStructure();
    mTLAS = new Render::TopLevelAccelerationStructure();

    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    mBLASCube->AddTriangles(mIndices, 0, cubeIndexCount, false, mVertices, 0, vertexCount, sizeof(Vertex), DXGI_FORMAT_R32G32B32_FLOAT, D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE);
    mBLASCube->PreBuild();

    mBLASPlane->AddTriangles(mIndices, cubeIndexCount, planeIndexCount, false, mVertices, 0, vertexCount, sizeof(Vertex), DXGI_FORMAT_R32G32B32_FLOAT, D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE);
    mBLASPlane->PreBuild();
   
    XMVECTOR vMove = { 0.0f, 3.0f, 0.0f, 0.0f };
    XMVECTOR yAxis = { 0.0f, 1.0f, 0.0f, 0.0f };
    XMMATRIX mRotate = XMMatrixRotationAxis(yAxis, XM_PIDIV4);
    XMMATRIX transformCube = mRotate * XMMatrixTranslationFromVector(vMove);

    XMMATRIX mMove = XMMatrixTranslation(-0.5f, 0.0f, -0.5f);
    XMMATRIX mScale = XMMatrixScaling(16.0f, 1.0f, 16.0f);
    XMMATRIX transformPlane = mMove * mScale * XMMatrixIdentity();
    
    mTLAS->AddInstance(mBLASCube, 0, 0, transformCube, 0xFF, D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE);
    mTLAS->AddInstance(mBLASPlane, 1, 0, transformPlane, 0xFF, D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE);
    mTLAS->PreBuild();

    Render::gCommand->Begin();
    Render::gCommand->BuildAccelerationStructure(mBLASCube);
    Render::gCommand->BuildAccelerationStructure(mBLASPlane);
    Render::gCommand->BuildAccelerationStructure(mTLAS);
    Render::gCommand->End(true);

    mBLASCube->PostBuild();
    mBLASPlane->PostBuild();
    mTLAS->PostBuild();
}

void DXRExample::CreateConstantBuffer(void) {
    mSceneConstantBuffer = new Render::ConstantBuffer(sizeof(SceneConstantBuffer), 1);
}

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void DXRExample::BuildShaderTables(void) {
    ID3D12StateObjectProperties *stateObjectProperties = nullptr;
    ASSERT_SUCCEEDED(mRayTracingState->Get()->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));

    // Get shader identifiers.
    void* rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(RaygenShaderName);
    void* missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(MissShaderName);
    void* missShadowIdentifier = stateObjectProperties->GetShaderIdentifier(MissShadowName);
    void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(HitCubeGroupName);
    void* hitGroupShadowIdentifier = stateObjectProperties->GetShaderIdentifier(HitShadowGroupName);
    uint32_t shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    // calculate buffer size
    uint32_t rayGenRecordSize = AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    uint32_t rayGenTableSize = AlignUp(rayGenRecordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    uint32_t missRecordSize = AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    uint32_t missTableSize = AlignUp(2 * missRecordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    uint32_t hitGroupShaderSize = AlignUp(shaderIdentifierSize + (uint32_t)(sizeof(mMeshConstBuf)), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    uint32_t hitGroupShadowSize = AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    uint32_t hitGroupeRecordSize = MAX(hitGroupShaderSize, hitGroupShadowSize);
    uint32_t hitGroupeTableSize = AlignUp(2 * hitGroupeRecordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    mShaderTable = new Render::UploadBuffer(rayGenTableSize + missTableSize + hitGroupeTableSize);
    uint32_t offset = 0;
    mShaderTable->UploadData(rayGenShaderIdentifier, shaderIdentifierSize, offset);
    offset += rayGenTableSize;
    mShaderTable->UploadData(missShaderIdentifier, shaderIdentifierSize, offset);
    mShaderTable->UploadData(missShadowIdentifier, shaderIdentifierSize, offset + missRecordSize);
    offset += missTableSize;
    mShaderTable->UploadData(hitGroupShaderIdentifier, shaderIdentifierSize, offset);
    mShaderTable->UploadData(&mMeshConstBuf, sizeof(mMeshConstBuf), offset + shaderIdentifierSize);
    mShaderTable->UploadData(hitGroupShadowIdentifier, shaderIdentifierSize, offset + hitGroupeRecordSize);

    mRaysDesc = {};
    // Since each shader table has only one shader record, the stride is same as the size.
    mRaysDesc.HitGroupTable.StartAddress = mShaderTable->GetGPUAddress() + rayGenTableSize + missTableSize;
    mRaysDesc.HitGroupTable.SizeInBytes = hitGroupeTableSize;
    mRaysDesc.HitGroupTable.StrideInBytes = hitGroupeRecordSize;
    mRaysDesc.MissShaderTable.StartAddress = mShaderTable->GetGPUAddress() + rayGenTableSize;
    mRaysDesc.MissShaderTable.SizeInBytes = missTableSize;
    mRaysDesc.MissShaderTable.StrideInBytes = missRecordSize;
    mRaysDesc.RayGenerationShaderRecord.StartAddress = mShaderTable->GetGPUAddress();
    mRaysDesc.RayGenerationShaderRecord.SizeInBytes = rayGenTableSize;
    mRaysDesc.Width = mWidth;
    mRaysDesc.Height = mHeight;
    mRaysDesc.Depth = 1;

    stateObjectProperties->Release();
}

// Create 2D output texture for raytracing.
void DXRExample::CreateRaytracingOutput(void) {
    DXGI_FORMAT format = Render::gRenderTarget[0]->GetFormat();
    uint32_t width = Render::gRenderTarget[0]->GetWidth();
    uint32_t height = Render::gRenderTarget[0]->GetHeight();

    // Create the output resource. The dimensions and format should match the swap-chain.
    mRaytracingOutput = new Render::PixelBuffer(width, width, height, format, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mRaytracingOutput->CreateUAV(mDescriptorHeap->Allocate());
}

// Update camera matrices passed into the shader.
void DXRExample::UpdateCameraMatrices(void) {
    XMVECTOR eye = { 0.0f, 10.0f, 10.0f, 1.0f };
    XMVECTOR at = { 0.0f, 0.0f, 0.0f, 1.0f };
    XMVECTOR up = { 0.0f, 1.0f, 0.0f, 0.0f };

    XMMATRIX view = XMMatrixLookAtRH(eye, at, up);
    XMMATRIX proj = XMMatrixPerspectiveFovRH(XM_PIDIV4, static_cast<float>(mWidth) / static_cast<float>(mHeight), 1.0f, 125.0f);
    XMMATRIX viewProj = view * proj;

    for (auto &sceneConstBuf : mSceneConstBuf) {
        sceneConstBuf.projectionToWorld = XMMatrixInverse(nullptr, viewProj);
    }
}

