#include "pch.h"
#include "AccelerationStructure.h"
#include "RenderCore.h"
#include "Resource/GPUBuffer.h"
#include "Resource/UploadBuffer.h"

namespace Render {

AccelerationStructure::AccelerationStructure(void)
: mResultData(nullptr)
, mScratchData(nullptr)
{

}

AccelerationStructure::~AccelerationStructure(void) {
    DeleteAndSetNull(mScratchData);
    DeleteAndSetNull(mResultData);
}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(void)
: AccelerationStructure()
{

}

BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure(void) {

}

void BottomLevelAccelerationStructure::AddTriangles(GPUBuffer *indices, uint64_t indexOffset, uint32_t indexCount, bool is16Bit, 
                                                    GPUBuffer *vertices, uint64_t vertexOffset, uint32_t vertexCount, uint64_t stride, DXGI_FORMAT format,
                                                    D3D12_RAYTRACING_GEOMETRY_FLAGS flags) {
    if (!indices || !indices->Get() || !vertices || !vertices->Get()) {
        return;
    }

    uint64_t indexOffsetBytes = (is16Bit ? sizeof(uint16_t) : sizeof(uint32_t)) * indexOffset;
    uint64_t vertexOffsetBytes = stride * vertexOffset;

    mGeometryDescs.PushBack({});
    D3D12_RAYTRACING_GEOMETRY_DESC &geometryDesc = mGeometryDescs.Last();
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Flags = flags;
    geometryDesc.Triangles.IndexBuffer = indices->GetGPUAddress() + indexOffsetBytes;
    geometryDesc.Triangles.IndexCount = indexCount;
    geometryDesc.Triangles.IndexFormat = is16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = format;
    geometryDesc.Triangles.VertexCount = vertexCount;
    geometryDesc.Triangles.VertexBuffer.StartAddress = vertices->GetGPUAddress() + vertexOffsetBytes;
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = stride;
}

bool BottomLevelAccelerationStructure::PreBuild(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags) {
    if (mGeometryDescs.Count() == 0) {
        return false;
    }

    mInputs = {};
    mInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    mInputs.Flags = flags;
    mInputs.NumDescs = mGeometryDescs.Count();
    mInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    mInputs.pGeometryDescs = mGeometryDescs.Data();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    gDXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&mInputs, &prebuildInfo);

    ASSERT_PRINT(prebuildInfo.ResultDataMaxSizeInBytes > 0);
    if (!prebuildInfo.ResultDataMaxSizeInBytes) {
        return false;
    }

    DeleteAndSetNull(mScratchData);
    DeleteAndSetNull(mResultData);

    mScratchData = new GPUBuffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mResultData = new GPUBuffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    return true;
}

void BottomLevelAccelerationStructure::PostBuild(void) {
    DeleteAndSetNull(mScratchData);
}

TopLevelAccelerationStructure::TopLevelAccelerationStructure(void)
: AccelerationStructure()
, mInstances(nullptr)
{

}

TopLevelAccelerationStructure::~TopLevelAccelerationStructure(void) {
    DeleteAndSetNull(mInstances);
}

void TopLevelAccelerationStructure::AddInstance(BottomLevelAccelerationStructure *blas, uint32_t id, uint32_t hitGroupIdx, XMMATRIX &transform, uint8_t idMask, uint8_t flags) {
    if (!blas || !blas->GetResult()) {
        return;
    }

    mInstanceDescs.PushBack({});
    D3D12_RAYTRACING_INSTANCE_DESC &instance = mInstanceDescs.Last();
    XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instance.Transform), transform);
    instance.InstanceID = id;
    instance.InstanceMask = idMask;
    instance.Flags = flags;
    instance.InstanceContributionToHitGroupIndex = hitGroupIdx;
    instance.AccelerationStructure = blas->GetResult()->GetGPUAddress();
}

bool TopLevelAccelerationStructure::PreBuild(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags) {
    if (!mInstanceDescs.Count()) {
        return false;
    }

    mInputs = {};
    mInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    mInputs.Flags = flags;
    mInputs.NumDescs = mInstanceDescs.Count();
    mInputs.pGeometryDescs = nullptr;
    mInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    gDXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&mInputs, &prebuildInfo);

    ASSERT_PRINT(prebuildInfo.ResultDataMaxSizeInBytes > 0);
    if (!prebuildInfo.ResultDataMaxSizeInBytes) {
        return false;
    }

    DeleteAndSetNull(mScratchData);
    DeleteAndSetNull(mResultData);
    DeleteAndSetNull(mInstances);

    mScratchData = new GPUBuffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mResultData = new GPUBuffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    uint64_t stride = AlignUp(sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);
    mInstances = new UploadBuffer(mInstanceDescs.Count() * stride);
    if (stride == sizeof(D3D12_RAYTRACING_INSTANCE_DESC)) {
        mInstances->UploadData(mInstanceDescs.Data(), mInstanceDescs.Count() * stride);
    } else {
        for (uint32_t i = 0; i < mInstanceDescs.Count(); ++i) {
            mInstances->UploadData(mInstanceDescs.Data() + i, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), i * stride);
        }
    }
    mInputs.InstanceDescs = mInstances->GetGPUAddress();

    return true;
}

void TopLevelAccelerationStructure::PostBuild(void) {
    DeleteAndSetNull(mScratchData);
    DeleteAndSetNull(mInstances);
}

}
