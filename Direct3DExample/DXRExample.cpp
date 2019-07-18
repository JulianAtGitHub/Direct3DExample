#include "pch.h"
#include "DXRExample.h"
#include "Core/Render/RenderCore.h"
#include "Core/Render/CommandContext.h"
#include "Core/Render/DescriptorHeap.h"
#include "Core/Render/Resource/GPUBuffer.h"
#include "Core/Render/Resource/UploadBuffer.h"
#include "Core/Render/Resource/ConstantBuffer.h"
#include "Core/Render/Resource/PixelBuffer.h"
#include "Core/Render/Resource/RenderTargetBuffer.h"

#include <iomanip>
#include <sstream>
#include <iostream>

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

// Pretty-print a state object tree.
inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
    std::wstringstream wstr;
    wstr << L"\n";
    wstr << L"--------------------------------------------------------------------\n";
    wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
    if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
    if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

    auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
    {
        std::wostringstream woss;
        for (UINT i = 0; i < numExports; i++)
        {
            woss << L"|";
            if (depth > 0)
            {
                for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
            }
            woss << L" [" << i << L"]: ";
            if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
            woss << exports[i].Name << L"\n";
        }
        return woss.str();
    };

    for (UINT i = 0; i < desc->NumSubobjects; i++)
    {
        wstr << L"| [" << i << L"]: ";
        switch (desc->pSubobjects[i].Type)
        {
        case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
            wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
            wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
            wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
        {
            wstr << L"DXIL Library 0x";
            auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
            wstr << ExportTree(1, lib->NumExports, lib->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
        {
            wstr << L"Existing Library 0x";
            auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << collection->pExistingCollection << L"\n";
            wstr << ExportTree(1, collection->NumExports, collection->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"Subobject to Exports Association (Subobject [";
            auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
            UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
            wstr << index << L"])\n";
            for (UINT j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"DXIL Subobjects to Exports Association (";
            auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
            wstr << association->SubobjectToAssociate << L")\n";
            for (UINT j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
        {
            wstr << L"Raytracing Shader Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
            wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
        {
            wstr << L"Raytracing Pipeline Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
        {
            wstr << L"Hit Group (";
            auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
            wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
            break;
        }
        }
        wstr << L"|--------------------------------------------------------------------\n";
    }
    wstr << L"\n";
    OutputDebugStringW(wstr.str().c_str());
}

const static wchar_t *HitCubeGroupName = L"MyHitGroup";
const static wchar_t *HitPlaneGroupName = L"MyHitPlaneGroup";
const static wchar_t *HitShadowGroupName = L"MyHitShadowGroup";
const static wchar_t *RaygenShaderName = L"MyRaygenShader";
const static wchar_t *ClosestHitShaderName = L"MyClosestHitShader";
const static wchar_t *ClosestHitPlaneName = L"MyClosestHitPlane";
const static wchar_t *ClosestHitShadowName = L"MyClosestHitShadow";
const static wchar_t *MissShaderName = L"MyMissShader";
const static wchar_t *MissShadowName = L"MyMissShadow";

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
        CubeConstantSlot = 0,
        Count
    };
}

DXRExample::DXRExample(HWND hwnd)
: Example(hwnd)
, mCurrentFrame(0)
, mGlobalRootSignature(nullptr)
, mLocalRootSignature(nullptr)
, mEmptyRootSignature(nullptr)
, mStateObject(nullptr)
, mDescriptorHeap(nullptr)
, mCubeVertices(nullptr)
, mCubeIndices(nullptr)
, mPlaneVertices(nullptr)
, mPlaneIndices(nullptr)
, mTopLevelAccelerationStructure(nullptr)
, mBottomLevelAccelerationStructure(nullptr)
, mBottomLevelAccelerationStructure2(nullptr)
, mSceneConstantBuffer(nullptr)
, mRayGenShaderTable(nullptr)
, mMissShaderTable(nullptr)
, mHitGroupShaderTable(nullptr)
, mRaytracingOutput(nullptr)
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
    //elapse += 1.0f;
    //if (elapse > 360.0f) {
    //    elapse -= 360.0f;
    //}

    float secondsToRotateAround = 8.0f;
    float angleToRotateBy = -elapse;
    XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
    XMVECTOR lightPosition = { 0.0f, 9.0f, -1.0f, 0.0f };
    mSceneConstBuf[mCurrentFrame].lightPosition = XMVector3Transform(lightPosition, rotate);
}

void DXRExample::Render(void) {
    Render::gCommand->Begin();
    ID3D12GraphicsCommandList4 *commandList = Render::gCommand->GetDXRCommandList();

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList->SetComputeRootSignature(mGlobalRootSignature);

    // Copy the updated scene constant buffer to GPU.
    void *mappedConstantData = mSceneConstantBuffer->GetMappedBuffer(0, mCurrentFrame);
    memcpy(mappedConstantData, mSceneConstBuf + mCurrentFrame, sizeof(SceneConstantBuffer));
    commandList->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantSlot, mSceneConstantBuffer->GetGPUAddress(0, mCurrentFrame));

    // Bind the heaps, acceleration structure and dispatch rays.
    Render::DescriptorHeap *heaps[] = { mDescriptorHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, 1);
    // Set index and successive vertex buffer decriptor tables
    commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, mCubeIndices->GetHandle().gpu);
    commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, mRaytracingOutput->GetHandle().gpu);
    commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, mTopLevelAccelerationStructure->GetGPUAddress());

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    // Since each shader table has only one shader record, the stride is same as the size.
    dispatchDesc.HitGroupTable.StartAddress = mHitGroupShaderTable->GetGPUAddress();
    dispatchDesc.HitGroupTable.SizeInBytes = mHitGroupShaderTable->GetBufferSize();
    dispatchDesc.HitGroupTable.StrideInBytes = mHitGroupShaderTable->GetBufferSize() / 3;
    dispatchDesc.MissShaderTable.StartAddress = mMissShaderTable->GetGPUAddress();
    dispatchDesc.MissShaderTable.SizeInBytes = mMissShaderTable->GetBufferSize();
    dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes / 2;
    dispatchDesc.RayGenerationShaderRecord.StartAddress = mRayGenShaderTable->GetGPUAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = mRayGenShaderTable->GetBufferSize();
    dispatchDesc.Width = mWidth;
    dispatchDesc.Height = mHeight;
    dispatchDesc.Depth = 1;
    commandList->SetPipelineState1(mStateObject);
    commandList->DispatchRays(&dispatchDesc);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_COPY_DEST);
    Render::gCommand->TransitResource(mRaytracingOutput, D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandList->CopyResource(Render::gRenderTarget[mCurrentFrame]->Get(), mRaytracingOutput->Get());

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);
    Render::gCommand->TransitResource(mRaytracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}

void DXRExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    DeleteAndSetNull(mRaytracingOutput);
    DeleteAndSetNull(mHitGroupShaderTable);
    DeleteAndSetNull(mMissShaderTable);
    DeleteAndSetNull(mRayGenShaderTable);
    DeleteAndSetNull(mSceneConstantBuffer);
    DeleteAndSetNull(mBottomLevelAccelerationStructure);
    DeleteAndSetNull(mBottomLevelAccelerationStructure2);
    DeleteAndSetNull(mTopLevelAccelerationStructure);
    DeleteAndSetNull(mCubeIndices);
    DeleteAndSetNull(mCubeVertices);
    DeleteAndSetNull(mPlaneIndices);
    DeleteAndSetNull(mPlaneVertices);
    DeleteAndSetNull(mDescriptorHeap);

    ReleaseAndSetNull(mStateObject);
    ReleaseAndSetNull(mEmptyRootSignature);
    ReleaseAndSetNull(mLocalRootSignature);
    ReleaseAndSetNull(mGlobalRootSignature);

    Render::Terminate();
}

void DXRExample::InitScene(void) {
    mCubeConstBuf.albedo = XMFLOAT4(1.0, 1.0, 1.0, 1.0);

    XMFLOAT4 cameraPosition(0.0f, 10.0f, 10.0f, 1.0f);
    XMFLOAT4 lightPosition(0.0f, 9.0f, -1.0f, 0.0f);
    XMFLOAT4 lightAmbientColor(0.5f, 0.5f, 0.5f, 1.0f);
    XMFLOAT4 lightDiffuseColor(0.5f, 0.0f, 0.0f, 1.0f);
    for (auto &sceneConstBuf : mSceneConstBuf) {
        sceneConstBuf.cameraPosition = XMLoadFloat4(&cameraPosition);
        sceneConstBuf.lightPosition = XMLoadFloat4(&lightPosition);
        sceneConstBuf.lightAmbientColor = XMLoadFloat4(&lightAmbientColor);
        sceneConstBuf.lightDiffuseColor = XMLoadFloat4(&lightDiffuseColor);
    }
}

void DXRExample::CreateRootSignature(void) {
    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[2]; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output texture / RenderTarget
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1);  // 2 static index and vertex buffers.

        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
        rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);
        rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
        rootParameters[GlobalRootSignatureParams::SceneConstantSlot].InitAsConstantBufferView(0);
        rootParameters[GlobalRootSignatureParams::VertexBuffersSlot].InitAsDescriptorTable(1, &ranges[1]);
        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(GlobalRootSignatureParams::Count, rootParameters);

        WRL::ComPtr<ID3DBlob> blob;
        WRL::ComPtr<ID3DBlob> error;
        ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
        ASSERT_SUCCEEDED(Render::gDXRDevice->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mGlobalRootSignature)));
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
        CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
        rootParameters[LocalRootSignatureParams::CubeConstantSlot].InitAsConstants(SizeOfInUint32(mCubeConstBuf), 1);
        CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(LocalRootSignatureParams::Count, rootParameters);
        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        WRL::ComPtr<ID3DBlob> blob;
        WRL::ComPtr<ID3DBlob> error;
        ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
        ASSERT_SUCCEEDED(Render::gDXRDevice->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mLocalRootSignature)));
    }

    // Empty Root Signature
    {
        D3D12_ROOT_SIGNATURE_DESC emptyRootSignatureDesc = {};
        emptyRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        WRL::ComPtr<ID3DBlob> blob;
        WRL::ComPtr<ID3DBlob> error;
        ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&emptyRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
        ASSERT_SUCCEEDED(Render::gDXRDevice->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mEmptyRootSignature)));
    }
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
    // 1 - Triangle hit group
    // 1 - Shader config
    // 2 - Local root signature and association
    // 1 - Global root signature
    // 1 - Pipeline config
    CList<D3D12_STATE_SUBOBJECT> subObjects;
    subObjects.Resize(13);

    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
    D3D12_DXIL_LIBRARY_DESC libDesc = {};
    libDesc.DXILLibrary.pShaderBytecode = ReadFileData("dxr.example.cso", libDesc.DXILLibrary.BytecodeLength);
    // Define which shader exports to surface from the library.
    // If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
    // In this sample, this could be ommited for convenience since the sample uses all shaders in the library. 
    constexpr uint32_t shaderCount = 6;
    D3D12_EXPORT_DESC libExports[shaderCount] = {
        { RaygenShaderName, nullptr, D3D12_EXPORT_FLAG_NONE },
        { ClosestHitShaderName, nullptr, D3D12_EXPORT_FLAG_NONE },
        { ClosestHitPlaneName, nullptr, D3D12_EXPORT_FLAG_NONE },
        { ClosestHitShadowName, nullptr, D3D12_EXPORT_FLAG_NONE },
        { MissShaderName, nullptr, D3D12_EXPORT_FLAG_NONE },
        { MissShadowName, nullptr, D3D12_EXPORT_FLAG_NONE },
    };
    libDesc.NumExports = shaderCount;
    libDesc.pExports = libExports;

    auto &lib = subObjects.At(0);
    lib.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    lib.pDesc = &libDesc;

    // Shader config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    D3D12_RAYTRACING_SHADER_CONFIG shaderConfigDesc = {};
    shaderConfigDesc.MaxPayloadSizeInBytes = sizeof(XMFLOAT4);    // float4 pixelColor
    shaderConfigDesc.MaxAttributeSizeInBytes = sizeof(XMFLOAT2);  // float2 barycentrics
    //
    auto &shaderConfig = subObjects.At(1);
    shaderConfig.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shaderConfig.pDesc = &shaderConfigDesc;

    // Triangle hit group
    // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
    // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.

    // Cube
    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.ClosestHitShaderImport = ClosestHitShaderName;
    hitGroupDesc.HitGroupExport = HitCubeGroupName;
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    //
    auto &hitGroup = subObjects.At(2);
    hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroup.pDesc = &hitGroupDesc;

    // Local root signature and shader association
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    D3D12_LOCAL_ROOT_SIGNATURE lrsDesc = { mLocalRootSignature };
    //
    auto &lrs = subObjects.At(3);
    lrs.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    lrs.pDesc = &lrsDesc;
    //
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportAssociationDesc = {};
    exportAssociationDesc.pSubobjectToAssociate = &lrs;
    exportAssociationDesc.NumExports = 1;
    exportAssociationDesc.pExports = &HitCubeGroupName;
    //
    auto &exportAssociation = subObjects.At(4);
    exportAssociation.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    exportAssociation.pDesc = &exportAssociationDesc;

    // Plane
    D3D12_HIT_GROUP_DESC hitGroupDesc2 = {};
    hitGroupDesc2.ClosestHitShaderImport = ClosestHitPlaneName;
    hitGroupDesc2.HitGroupExport = HitPlaneGroupName;
    hitGroupDesc2.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    //
    auto &hitGroup2 = subObjects.At(5);
    hitGroup2.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroup2.pDesc = &hitGroupDesc2;

    //
    D3D12_LOCAL_ROOT_SIGNATURE lrsDesc2 = { mEmptyRootSignature };
    //
    auto &lrs2 = subObjects.At(6);
    lrs2.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    lrs2.pDesc = &lrsDesc2;
    //
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportAssociationDesc2 = {};
    exportAssociationDesc2.pSubobjectToAssociate = &lrs2;
    exportAssociationDesc2.NumExports = 1;
    exportAssociationDesc2.pExports = &HitPlaneGroupName;
    //
    auto &exportAssociation2 = subObjects.At(7);
    exportAssociation2.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    exportAssociation2.pDesc = &exportAssociationDesc2;

    // Shadow
    D3D12_HIT_GROUP_DESC hitGroupDesc3 = {};
    hitGroupDesc3.ClosestHitShaderImport = ClosestHitShadowName;
    hitGroupDesc3.HitGroupExport = HitShadowGroupName;
    hitGroupDesc3.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    //
    auto &hitGroup3 = subObjects.At(8);
    hitGroup3.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroup3.pDesc = &hitGroupDesc3;

    //
    D3D12_LOCAL_ROOT_SIGNATURE lrsDesc3 = { mEmptyRootSignature };
    //
    auto &lrs3 = subObjects.At(9);
    lrs3.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    lrs3.pDesc = &lrsDesc3;
    //
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportAssociationDesc3 = {};
    exportAssociationDesc3.pSubobjectToAssociate = &lrs3;
    exportAssociationDesc3.NumExports = 1;
    exportAssociationDesc3.pExports = &HitShadowGroupName;
    //
    auto &exportAssociation3 = subObjects.At(10);
    exportAssociation3.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    exportAssociation3.pDesc = &exportAssociationDesc3;

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    D3D12_GLOBAL_ROOT_SIGNATURE grsDesc = { mGlobalRootSignature };
    //
    auto &grs = subObjects.At(11);
    grs.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    grs.pDesc = &grsDesc;

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfigDesc = {};
    // PERFOMANCE TIP: Set max recursion depth as low as needed 
    // as drivers may apply optimization strategies for low recursion depths.
    pipelineConfigDesc.MaxTraceRecursionDepth = 2; // ~ primary rays only.
    //
    auto &pipelineConfig = subObjects.At(12);
    pipelineConfig.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    pipelineConfig.pDesc = &pipelineConfigDesc;


    D3D12_STATE_OBJECT_DESC raytracingPipeline = {};
    raytracingPipeline.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    raytracingPipeline.NumSubobjects = subObjects.Count();
    raytracingPipeline.pSubobjects = subObjects.Data();

#if _DEBUG
    PrintStateObjectDesc(&raytracingPipeline);
#endif

    ASSERT_SUCCEEDED(Render::gDXRDevice->CreateStateObject(&raytracingPipeline, IID_PPV_ARGS(&mStateObject)));

    free((void *)(libDesc.DXILLibrary.pShaderBytecode));
}

void DXRExample::CreateDescriptorHeap(void) {
    // Allocate a heap for 5 descriptors:
    // 2 - vertex and index buffer SRVs
    // 1 - raytracing output texture SRV
    mDescriptorHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 5);
}

void DXRExample::BuildGeometry(void) {
    Render::gCommand->Begin();

    // Cube
    {
        uint16_t indices[] = {
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
            22,20,23
        };

        // Cube vertices positions and corresponding triangle normals.
        Vertex vertices[] = {
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
        };

        mCubeIndices = new Render::GPUBuffer(sizeof(indices));
        mCubeVertices = new Render::GPUBuffer(sizeof(vertices));

        Render::gCommand->UploadBuffer(mCubeIndices, 0, indices, sizeof(indices));
        Render::gCommand->UploadBuffer(mCubeVertices, 0, vertices, sizeof(vertices));

        mCubeIndices->CreateIndexBufferSRV(mDescriptorHeap->Allocate(), sizeof(indices) / 4);
        mCubeVertices->CreateVertexBufferSRV(mDescriptorHeap->Allocate(), ARRAYSIZE(vertices), sizeof(Vertex));
    }

    // Plane
    {
        uint16_t indices[] = {
            0,1,3,
            3,1,2,
        };

        // Cube vertices positions and corresponding triangle normals.
        Vertex vertices[] = {
            { XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        };

        mPlaneIndices = new Render::GPUBuffer(sizeof(indices));
        mPlaneVertices = new Render::GPUBuffer(sizeof(vertices));

        Render::gCommand->UploadBuffer(mPlaneVertices, 0, vertices, sizeof(vertices));
        Render::gCommand->UploadBuffer(mPlaneIndices, 0, indices, sizeof(indices));

        mPlaneIndices->CreateIndexBufferSRV(mDescriptorHeap->Allocate(), sizeof(indices) / 4);
        mPlaneVertices->CreateVertexBufferSRV(mDescriptorHeap->Allocate(), ARRAYSIZE(vertices), sizeof(Vertex));
    }

    Render::gCommand->End(true);
}

void DXRExample::BuildAccelerationStructure(void) {
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDescs[2] = { {}, {} };
    geometryDescs[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDescs[0].Triangles.IndexBuffer = mCubeIndices->GetGPUAddress();
    geometryDescs[0].Triangles.IndexCount = (UINT)mCubeIndices->GetBufferSize() / sizeof(uint16_t);
    geometryDescs[0].Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDescs[0].Triangles.Transform3x4 = 0;
    geometryDescs[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDescs[0].Triangles.VertexCount = (UINT)mCubeVertices->GetBufferSize() / sizeof(Vertex);
    geometryDescs[0].Triangles.VertexBuffer.StartAddress = mCubeVertices->GetGPUAddress();
    geometryDescs[0].Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDescs[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    geometryDescs[1].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDescs[1].Triangles.IndexBuffer = mPlaneIndices->GetGPUAddress();
    geometryDescs[1].Triangles.IndexCount = (UINT)mPlaneIndices->GetBufferSize() / sizeof(uint16_t);
    geometryDescs[1].Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDescs[1].Triangles.Transform3x4 = 0;
    geometryDescs[1].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDescs[1].Triangles.VertexCount = (UINT)mPlaneVertices->GetBufferSize() / sizeof(Vertex);
    geometryDescs[1].Triangles.VertexBuffer.StartAddress = mPlaneVertices->GetGPUAddress();
    geometryDescs[1].Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    geometryDescs[1].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDescs[2] = { {}, {} };
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &bottomLevelInputs0 = bottomLevelBuildDescs[0].Inputs;
    bottomLevelInputs0.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs0.Flags = buildFlags;
    bottomLevelInputs0.NumDescs = 1;
    bottomLevelInputs0.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs0.pGeometryDescs = geometryDescs;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &bottomLevelInputs1 = bottomLevelBuildDescs[1].Inputs;
    bottomLevelInputs1.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs1.Flags = buildFlags;
    bottomLevelInputs1.NumDescs = 1;
    bottomLevelInputs1.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs1.pGeometryDescs = geometryDescs + 1;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 2;
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    Render::gDXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    ASSERT_PRINT(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfos[2] = { {}, {} };
    Render::gDXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs0, bottomLevelPrebuildInfos);
    ASSERT_PRINT(bottomLevelPrebuildInfos[0].ResultDataMaxSizeInBytes > 0);
    Render::gDXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs1, bottomLevelPrebuildInfos + 1);
    ASSERT_PRINT(bottomLevelPrebuildInfos[1].ResultDataMaxSizeInBytes > 0);

    Render::GPUBuffer *bottomLevelScratch0 = new Render::GPUBuffer(bottomLevelPrebuildInfos[0].ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Render::GPUBuffer *bottomLevelScratch1 = new Render::GPUBuffer(bottomLevelPrebuildInfos[1].ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Render::GPUBuffer *topLevelScratch = new Render::GPUBuffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesn’t need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    mBottomLevelAccelerationStructure = new Render::GPUBuffer(bottomLevelPrebuildInfos[0].ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mBottomLevelAccelerationStructure2 = new Render::GPUBuffer(bottomLevelPrebuildInfos[1].ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mTopLevelAccelerationStructure = new Render::GPUBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // Create an instance desc for the bottom-level acceleration structure.
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc[2] = { {}, {} };

    {
        XMVECTOR vMove = { 0.0f, 2.0f, 0.0f, 0.0f };
        XMVECTOR yAxis = { 0.0f, 1.0f, 0.0f, 0.0f };
        XMMATRIX mRotate = XMMatrixRotationAxis(yAxis, XM_PIDIV4);
        XMMATRIX transform = mRotate * XMMatrixTranslationFromVector(vMove);
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc[0].Transform), transform);
    }
    instanceDesc[0].InstanceID = 0;
    instanceDesc[0].InstanceMask = 1;
    instanceDesc[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc[0].InstanceContributionToHitGroupIndex = 0;
    instanceDesc[0].AccelerationStructure = mBottomLevelAccelerationStructure->GetGPUAddress();

    {
        XMMATRIX mMove = XMMatrixTranslation(-0.5f, 0.0f, -0.5f);
        XMMATRIX mScale = XMMatrixScaling(16.0f, 1.0f, 16.0f);
        XMMATRIX transform2 = mMove * mScale * XMMatrixIdentity();
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc[1].Transform), transform2);
    }
    instanceDesc[1].InstanceID = 1;
    instanceDesc[1].InstanceMask = 1;
    instanceDesc[1].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc[1].InstanceContributionToHitGroupIndex = 1;
    instanceDesc[1].AccelerationStructure = mBottomLevelAccelerationStructure2->GetGPUAddress();

    //D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT
    size_t instanceBufferSize = 2 * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    Render::UploadBuffer *instanceDescs = new Render::UploadBuffer(instanceBufferSize);
    instanceDescs->UploadData(instanceDesc, instanceBufferSize);

    // Bottom Level Acceleration Structure desc
    bottomLevelBuildDescs[0].ScratchAccelerationStructureData = bottomLevelScratch0->GetGPUAddress();
    bottomLevelBuildDescs[0].DestAccelerationStructureData = mBottomLevelAccelerationStructure->GetGPUAddress();
    bottomLevelBuildDescs[1].ScratchAccelerationStructureData = bottomLevelScratch1->GetGPUAddress();
    bottomLevelBuildDescs[1].DestAccelerationStructureData = mBottomLevelAccelerationStructure2->GetGPUAddress();

    // Top Level Acceleration Structure desc
    topLevelBuildDesc.DestAccelerationStructureData = mTopLevelAccelerationStructure->GetGPUAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = topLevelScratch->GetGPUAddress();
    topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUAddress();

    Render::gCommand->Begin();

    Render::gCommand->GetDXRCommandList()->BuildRaytracingAccelerationStructure(bottomLevelBuildDescs, 0, nullptr);
    Render::gCommand->InsertUAVBarrier(mBottomLevelAccelerationStructure);
    Render::gCommand->GetDXRCommandList()->BuildRaytracingAccelerationStructure(bottomLevelBuildDescs + 1, 0, nullptr);
    Render::gCommand->InsertUAVBarrier(mBottomLevelAccelerationStructure2);

    Render::gCommand->GetDXRCommandList()->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    Render::gCommand->InsertUAVBarrier(mTopLevelAccelerationStructure);

    Render::gCommand->End(true);

    DeleteAndSetNull(bottomLevelScratch0);
    DeleteAndSetNull(bottomLevelScratch1);
    DeleteAndSetNull(topLevelScratch);
    DeleteAndSetNull(instanceDescs);
}

void DXRExample::CreateConstantBuffer(void) {
    mSceneConstantBuffer = new Render::ConstantBuffer(sizeof(SceneConstantBuffer), 1);
}

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void DXRExample::BuildShaderTables(void) {
    ID3D12StateObjectProperties *stateObjectProperties = nullptr;
    ASSERT_SUCCEEDED(mStateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));

    // Get shader identifiers.
    void* rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(RaygenShaderName);
    void* missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(MissShaderName);
    void* missShadowIdentifier = stateObjectProperties->GetShaderIdentifier(MissShadowName);
    void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(HitCubeGroupName);
    void* hitGroupPlaneIdentifier = stateObjectProperties->GetShaderIdentifier(HitPlaneGroupName);
    void* hitGroupShadowIdentifier = stateObjectProperties->GetShaderIdentifier(HitShadowGroupName);
    uint32_t shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    // Ray gen shader table
    {
        uint32_t numShaderRecords = 1;
        uint32_t shaderRecordSize = AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mRayGenShaderTable = new Render::UploadBuffer(numShaderRecords * shaderRecordSize);
        mRayGenShaderTable->UploadData(rayGenShaderIdentifier, shaderIdentifierSize);
    }

    // Miss shader table
    {
        uint32_t numShaderRecords = 2;
        uint32_t shaderRecordSize = AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mMissShaderTable = new Render::UploadBuffer(numShaderRecords * shaderRecordSize);
        mMissShaderTable->UploadData(missShaderIdentifier, shaderIdentifierSize);
        mMissShaderTable->UploadData(missShadowIdentifier, shaderIdentifierSize, shaderRecordSize);
    }

    // Hit group shader table
    {
        uint32_t hitGroupShaderSize = AlignUp(shaderIdentifierSize + (uint32_t)(sizeof(mCubeConstBuf)), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        uint32_t hitPlaneShaderSize = AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        uint32_t elementSize = MAX(hitGroupShaderSize, hitPlaneShaderSize);
        mHitGroupShaderTable = new Render::UploadBuffer(3 * elementSize);
        mHitGroupShaderTable->UploadData(hitGroupShaderIdentifier, shaderIdentifierSize);
        mHitGroupShaderTable->UploadData(&mCubeConstBuf, sizeof(mCubeConstBuf), shaderIdentifierSize);
        mHitGroupShaderTable->UploadData(hitGroupPlaneIdentifier, shaderIdentifierSize, elementSize);
        mHitGroupShaderTable->UploadData(hitGroupShadowIdentifier, shaderIdentifierSize, 2 * elementSize);
        
    }

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

