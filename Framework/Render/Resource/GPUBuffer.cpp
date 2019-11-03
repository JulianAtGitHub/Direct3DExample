#include "stdafx.h"
#include "GPUBuffer.h"

namespace Render {

GPUBuffer::GPUBuffer(size_t size, D3D12_RESOURCE_STATES usage, D3D12_RESOURCE_FLAGS flag, bool cpuWritable)
: GPUResource()
, mBufferSize(size)
, mFlag(flag)
, mCpuWritable(cpuWritable)
, mMappedBuffer(nullptr)
{
    SetUsageState(usage);
    Initialize();
}

GPUBuffer::~GPUBuffer(void) {
    if (mCpuWritable) {
        mResource->Unmap(0, nullptr);
    }
    mBufferSize = 0;
}

void GPUBuffer::Initialize(void) {
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Alignment = 0;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Flags = mFlag;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Height = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Width = mBufferSize;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = mCpuWritable ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, mUsageState, nullptr, IID_PPV_ARGS(&mResource)));
    mResource->SetName(L"GPUBuffer");

    FillGPUAddress();

    if (mCpuWritable) {
        CD3DX12_RANGE readRange(0, 0);
        ASSERT_SUCCEEDED(mResource->Map(0, &readRange, &mMappedBuffer));
    }
}

void GPUBuffer::CreateStructBufferSRV(const DescriptorHandle &handle, uint32_t count, uint32_t size) {
    if (!mResource) {
        return;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.NumElements = count;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.StructureByteStride = size;
    gDevice->CreateShaderResourceView(mResource, &srvDesc, handle.cpu);

    mHandle = handle;
}

void GPUBuffer::CreateRawBufferSRV(const DescriptorHandle &handle, uint32_t count) {
    if (!mResource) {
        return;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.Buffer.NumElements = count;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    srvDesc.Buffer.StructureByteStride = 0;
    gDevice->CreateShaderResourceView(mResource, &srvDesc, handle.cpu);

    mHandle = handle;
}

void GPUBuffer::UploadData(const void *data, size_t size, uint64_t offset) {
    ASSERT_PRINT(mCpuWritable, "This buffer can not write by cpu, call CommandContext::UploadBuffer instead!");

    if (!mCpuWritable || !data) {
        return;
    }

    if (offset + size > mBufferSize) {
        size = mBufferSize - offset;
    }

    uint8_t *dest = (uint8_t *)mMappedBuffer;
    memcpy(dest + offset, data, size);
}

}
