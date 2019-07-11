#include "pch.h"
#include "DXRExample.h"
#include "Core/Render/RenderCore.h"
#include "Core/Render/CommandContext.h"
#include "Core/Render/Resource/GPUBuffer.h"
#include "Core/Render/Resource/UploadBuffer.h"

const static XMFLOAT3 vertices[] = {
    XMFLOAT3(0,          1,  0),
    XMFLOAT3(0.866f,  -0.5f, 0),
    XMFLOAT3(-0.866f, -0.5f, 0),
};

DXRExample::DXRExample(HWND hwnd)
: Example(hwnd)
, mVertexBuffer(nullptr)
, mBLAS(nullptr)
, mTLAS(nullptr)
, mTlasSize(0)
, mCurrentFrame(0)
{

}

DXRExample::~DXRExample(void) {

}

void DXRExample::Init(void) {
    Render::Initialize(mHwnd);
    InitAccelerationStructures();
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
}

void DXRExample::Update(void) {

}

void DXRExample::Render(void) {

}

void DXRExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    DeleteAndSetNull(mBLAS);
    DeleteAndSetNull(mTLAS);
    DeleteAndSetNull(mVertexBuffer);

    Render::Terminate();
}

void DXRExample::InitAccelerationStructures(void) {
    uint32_t bufferSize = sizeof(XMFLOAT3) * _countof(vertices);
    mVertexBuffer = new Render::GPUBuffer(AlignUp(bufferSize, 256));

    Render::gCommand->Begin();
    Render::gCommand->UploadBuffer(mVertexBuffer, 0, vertices, bufferSize);

    // Bottom level acceleration structure
    Render::GPUBuffer *scratchBLAS = nullptr;
    Render::GPUBuffer *resultBLAS = nullptr;
    {
        D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
        geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geomDesc.Triangles.VertexBuffer.StartAddress = mVertexBuffer->GetGPUAddress();
        geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(XMFLOAT3);
        geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geomDesc.Triangles.VertexCount = _countof(vertices);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        inputs.NumDescs = 1;
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.pGeometryDescs = &geomDesc;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild = {};
        Render::gDevice5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);

        scratchBLAS = new Render::GPUBuffer((uint32_t)prebuild.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        resultBLAS = new Render::GPUBuffer((uint32_t)prebuild.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
        desc.Inputs = inputs;
        desc.DestAccelerationStructureData = resultBLAS->GetGPUAddress();
        desc.ScratchAccelerationStructureData = scratchBLAS->GetGPUAddress();

        Render::gCommand->GetCommandList4()->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

        Render::gCommand->InsertUAVBarrier(resultBLAS);
    }

    // Top level acceleration structure
    Render::GPUBuffer *scratchTLAS = nullptr;
    Render::GPUBuffer *resultTLAS = nullptr;
    Render::UploadBuffer *instanceDescTLAS = nullptr;
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        inputs.NumDescs = 1;
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild = {};
        Render::gDevice5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);

        scratchTLAS = new Render::GPUBuffer((uint32_t)prebuild.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        resultTLAS = new Render::GPUBuffer((uint32_t)prebuild.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        mTlasSize = prebuild.ResultDataMaxSizeInBytes;

        instanceDescTLAS = new Render::UploadBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        D3D12_RAYTRACING_INSTANCE_DESC instance = {};
        // Initialize the instance desc. We only have a single instance
        instance.InstanceID = 0;                            // This value will be exposed to the shader via InstanceID()
        instance.InstanceContributionToHitGroupIndex = 0;   // This is the offset inside the shader-table. We only have a single geometry, so the offset 0
        instance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        XMMATRIX m = DirectX::XMMatrixIdentity(); // Identity matrix
        memcpy(instance.Transform, &m, sizeof(instance.Transform));
        instance.AccelerationStructure = resultBLAS->GetGPUAddress();
        instance.InstanceMask = 0xFF;
        instanceDescTLAS->UploadData(&instance, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
        desc.Inputs = inputs;
        desc.Inputs.InstanceDescs = instanceDescTLAS->GetGPUAddress();
        desc.DestAccelerationStructureData = resultTLAS->GetGPUAddress();
        desc.ScratchAccelerationStructureData = scratchTLAS->GetGPUAddress();

        Render::gCommand->GetCommandList4()->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

        Render::gCommand->InsertUAVBarrier(resultTLAS);
    }

    Render::gCommand->End(true);

    mBLAS = resultBLAS;
    mTLAS = resultTLAS;

    DeleteAndSetNull(scratchBLAS);
    DeleteAndSetNull(scratchTLAS);
    DeleteAndSetNull(instanceDescTLAS);
}

