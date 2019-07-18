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

const static wchar_t *HitGroupName = L"MyHitGroup";
const static wchar_t *RaygenShaderName = L"MyRaygenShader";
const static wchar_t *ClosestHitShaderName = L"MyClosestHitShader";
const static wchar_t *MissShaderName = L"MyMissShader";

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

DXRExample::DXRExample(HWND hwnd)
: Example(hwnd)
, mCurrentFrame(0)
, mGlobalRootSignature(nullptr)
, mLocalRootSignature(nullptr)
, mStateObject(nullptr)
, mDescriptorHeap(nullptr)
, mVertexBuffer(nullptr)
, mIndexBuffer(nullptr)
, mBottomLevelAccelerationStructure(nullptr)
, mTopLevelAccelerationStructure(nullptr)
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
    elapse += 1.0f;
    if (elapse > 360.0f) {
        elapse -= 360.0f;
    }

    float secondsToRotateAround = 8.0f;
    float angleToRotateBy = -elapse;
    XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
    XMVECTOR lightPosition = { 0.0f, 1.8f, -3.0f, 0.0f };
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
    commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, mIndexBuffer->GetHandle().gpu);
    commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, mRaytracingOutput->GetHandle().gpu);
    commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, mTopLevelAccelerationStructure->GetGPUAddress());

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    // Since each shader table has only one shader record, the stride is same as the size.
    dispatchDesc.HitGroupTable.StartAddress = mHitGroupShaderTable->GetGPUAddress();
    dispatchDesc.HitGroupTable.SizeInBytes = mHitGroupShaderTable->GetBufferSize();
    dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
    dispatchDesc.MissShaderTable.StartAddress = mMissShaderTable->GetGPUAddress();
    dispatchDesc.MissShaderTable.SizeInBytes = mMissShaderTable->GetBufferSize();
    dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
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
    DeleteAndSetNull(mTopLevelAccelerationStructure);
    DeleteAndSetNull(mBottomLevelAccelerationStructure);
    DeleteAndSetNull(mIndexBuffer);
    DeleteAndSetNull(mVertexBuffer);
    DeleteAndSetNull(mDescriptorHeap);

    ReleaseAndSetNull(mStateObject)
    ReleaseAndSetNull(mLocalRootSignature);
    ReleaseAndSetNull(mGlobalRootSignature);

    Render::Terminate();
}

void DXRExample::InitScene(void) {
    mCubeConstBuf.albedo = XMFLOAT4(1.0, 1.0, 1.0, 1.0);

    XMFLOAT4 cameraPosition(0.0f, 2.0f, -5.0f, 1.0f);
    XMFLOAT4 lightPosition(0.0f, 1.8f, -3.0f, 0.0f);
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
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);  // 2 static index and vertex buffers.

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
    subObjects.Resize(7);

    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
    D3D12_DXIL_LIBRARY_DESC libDesc = {};
    libDesc.DXILLibrary.pShaderBytecode = ReadFileData("dxr.example.cso", libDesc.DXILLibrary.BytecodeLength);
    // Define which shader exports to surface from the library.
    // If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
    // In this sample, this could be ommited for convenience since the sample uses all shaders in the library. 
    D3D12_EXPORT_DESC libExports[3] = {
        { RaygenShaderName, nullptr, D3D12_EXPORT_FLAG_NONE },
        { ClosestHitShaderName, nullptr, D3D12_EXPORT_FLAG_NONE },
        { MissShaderName, nullptr, D3D12_EXPORT_FLAG_NONE },
    };
    libDesc.NumExports = 3;
    libDesc.pExports = libExports;

    auto &lib = subObjects.At(0);
    lib.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    lib.pDesc = &libDesc;

    // Triangle hit group
    // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
    // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.ClosestHitShaderImport = ClosestHitShaderName;
    hitGroupDesc.HitGroupExport = HitGroupName;
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    //
    auto &hitGroup = subObjects.At(1);
    hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroup.pDesc = &hitGroupDesc;

    // Shader config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    D3D12_RAYTRACING_SHADER_CONFIG shaderConfigDesc = {};
    shaderConfigDesc.MaxPayloadSizeInBytes = sizeof(XMFLOAT4);    // float4 pixelColor
    shaderConfigDesc.MaxAttributeSizeInBytes = sizeof(XMFLOAT2);  // float2 barycentrics
    //
    auto &shaderConfig = subObjects.At(2);
    shaderConfig.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shaderConfig.pDesc = &shaderConfigDesc;

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
    exportAssociationDesc.pExports = &HitGroupName;
    //
    auto &exportAssociation = subObjects.At(4);
    exportAssociation.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    exportAssociation.pDesc = &exportAssociationDesc;

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    D3D12_GLOBAL_ROOT_SIGNATURE grsDesc = { mGlobalRootSignature };
    //
    auto &grs = subObjects.At(5);
    grs.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    grs.pDesc = &grsDesc;

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfigDesc = {};
    // PERFOMANCE TIP: Set max recursion depth as low as needed 
    // as drivers may apply optimization strategies for low recursion depths.
    pipelineConfigDesc.MaxTraceRecursionDepth = 1; // ~ primary rays only.
    //
    auto &pipelineConfig = subObjects.At(6);
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
    mDescriptorHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3);
}

void DXRExample::BuildGeometry(void) {
    // Cube indices.
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

    Render::gCommand->Begin();

    mIndexBuffer = new Render::GPUBuffer(sizeof(indices));
    mVertexBuffer = new Render::GPUBuffer(sizeof(vertices));

    Render::gCommand->UploadBuffer(mIndexBuffer, 0, indices, sizeof(indices));
    Render::gCommand->UploadBuffer(mVertexBuffer, 0, vertices, sizeof(vertices));

    mIndexBuffer->CreateIndexBufferSRV(mDescriptorHeap->Allocate(), sizeof(indices) / 4);
    mVertexBuffer->CreateVertexBufferSRV(mDescriptorHeap->Allocate(), ARRAYSIZE(vertices), sizeof(Vertex));

    Render::gCommand->End(true);
}

void DXRExample::BuildAccelerationStructure(void) {
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = mIndexBuffer->GetGPUAddress();
    geometryDesc.Triangles.IndexCount = (UINT)mIndexBuffer->GetBufferSize() / sizeof(uint16_t);
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = (UINT)mVertexBuffer->GetBufferSize() / sizeof(Vertex);
    geometryDesc.Triangles.VertexBuffer.StartAddress = mVertexBuffer->GetGPUAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags = buildFlags;
    bottomLevelInputs.NumDescs = 1;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = bottomLevelBuildDesc;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    Render::gDXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    ASSERT_PRINT(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    Render::gDXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    ASSERT_PRINT(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    Render::GPUBuffer *bottomLevelScratch = new Render::GPUBuffer(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Render::GPUBuffer *topLevelScratch = new Render::GPUBuffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesn’t need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    mBottomLevelAccelerationStructure = new Render::GPUBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mTopLevelAccelerationStructure = new Render::GPUBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // Create an instance desc for the bottom-level acceleration structure.
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc.AccelerationStructure = mBottomLevelAccelerationStructure->GetGPUAddress();
    Render::UploadBuffer *instanceDescs = new Render::UploadBuffer(sizeof(instanceDesc));
    instanceDescs->UploadData(&instanceDesc, sizeof(instanceDesc));

    // Bottom Level Acceleration Structure desc
    bottomLevelBuildDesc.ScratchAccelerationStructureData = bottomLevelScratch->GetGPUAddress();
    bottomLevelBuildDesc.DestAccelerationStructureData = mBottomLevelAccelerationStructure->GetGPUAddress();

    // Top Level Acceleration Structure desc
    topLevelBuildDesc.DestAccelerationStructureData = mTopLevelAccelerationStructure->GetGPUAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = topLevelScratch->GetGPUAddress();
    topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUAddress();

    Render::gCommand->Begin();

    Render::gCommand->GetDXRCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
    Render::gCommand->InsertUAVBarrier(mBottomLevelAccelerationStructure);

    Render::gCommand->GetDXRCommandList()->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    Render::gCommand->InsertUAVBarrier(mTopLevelAccelerationStructure);

    Render::gCommand->End(true);

    DeleteAndSetNull(bottomLevelScratch);
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
    void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(HitGroupName);
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
        uint32_t numShaderRecords = 1;
        uint32_t shaderRecordSize = AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mMissShaderTable = new Render::UploadBuffer(numShaderRecords * shaderRecordSize);
        mMissShaderTable->UploadData(missShaderIdentifier, shaderIdentifierSize);
    }

    // Hit group shader table
    {
        uint32_t numShaderRecords = 1;
        uint32_t shaderRecordSize = AlignUp(shaderIdentifierSize + (uint32_t)(sizeof(mCubeConstBuf)), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mHitGroupShaderTable = new Render::UploadBuffer(numShaderRecords * shaderRecordSize);
        mHitGroupShaderTable->UploadData(hitGroupShaderIdentifier, shaderIdentifierSize);
        mHitGroupShaderTable->UploadData(&mCubeConstBuf, sizeof(mCubeConstBuf), shaderIdentifierSize);
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
    XMVECTOR eye = { 0.0f, 2.0f, -5.0f, 1.0f };
    XMVECTOR at = { 0.0f, 0.0f, 0.0f, 1.0f };
    XMVECTOR right = { 1.0f, 0.0f, 0.0f, 0.0f };
    XMVECTOR direction = XMVector4Normalize(at - eye);
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(direction, right));

    float fovAngleY = 45.0f;
    XMMATRIX view = XMMatrixLookAtRH(eye, at, up);
    XMMATRIX proj = XMMatrixPerspectiveFovRH(XMConvertToRadians(fovAngleY), static_cast<float>(mWidth) / static_cast<float>(mHeight), 1.0f, 125.0f);
    XMMATRIX viewProj = view * proj;

    for (auto &sceneConstBuf : mSceneConstBuf) {
        sceneConstBuf.projectionToWorld = XMMatrixInverse(nullptr, viewProj);
    }
}

