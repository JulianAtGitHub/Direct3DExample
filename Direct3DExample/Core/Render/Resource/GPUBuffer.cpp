#include "pch.h"
#include "GPUBuffer.h"
#include "Core/Render/RenderCore.h"

namespace Render {

GPUBuffer::GPUBuffer(size_t size, D3D12_RESOURCE_STATES usage, D3D12_RESOURCE_FLAGS flag)
: GPUResource()
, mBufferSize(size)
, mFlag(flag)
{
    SetUsageState(usage);
    Initialize();
}

GPUBuffer::~GPUBuffer(void) {
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
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, mUsageState, nullptr, IID_PPV_ARGS(&mResource)));
    mResource->SetName(L"GPUBuffer");

    FillGPUAddress();
}

void GPUBuffer::CreateVertexBufferSRV(DescriptorHandle &handle, uint32_t count, uint32_t size) {
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

void GPUBuffer::CreateIndexBufferSRV(DescriptorHandle &handle, uint32_t count) {
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

}
