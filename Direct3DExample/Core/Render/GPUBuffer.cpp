#include "pch.h"
#include "GPUBuffer.h"
#include "RenderCore.h"

namespace Render {

GPUBuffer::GPUBuffer(uint32_t size, D3D12_RESOURCE_STATES usage)
: GPUResource()
, mBufferSize(size)
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
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
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

}
