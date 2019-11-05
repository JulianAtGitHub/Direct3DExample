#include "stdafx.h"
#include "PixelBuffer.h"

namespace Render {

PixelBuffer::PixelBuffer(uint32_t pitch, uint32_t width, uint32_t height, uint32_t mipLevels, DXGI_FORMAT format, D3D12_RESOURCE_STATES usage, D3D12_RESOURCE_FLAGS flag)
: GPUResource()
, mPitch(pitch)
, mWidth(width)
, mHeight(height)
, mMipLevels(mipLevels)
, mFormat(format)
, mFlag(flag)
{
    SetUsageState(usage);
    Initialize();
}

PixelBuffer::PixelBuffer(void)
: GPUResource()
, mWidth(0)
, mHeight(0)
, mFormat(DXGI_FORMAT_UNKNOWN)
, mFlag(D3D12_RESOURCE_FLAG_NONE)
{

}

PixelBuffer::~PixelBuffer(void) {

}

void PixelBuffer::Initialize(void) {
    if (!mWidth || !mHeight || mFormat == DXGI_FORMAT_UNKNOWN) {
        return;
    }

    mUAVHandles.resize(mMipLevels);

    mUsageState = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = mMipLevels;
    texDesc.Format = mFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = mFlag;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, mUsageState, nullptr, IID_PPV_ARGS(&mResource)));
    mResource->SetName(L"Texture");
}

void PixelBuffer::CreateSRV(const DescriptorHandle &handle) {
    if (!mResource) {
        return;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mMipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    gDevice->CreateShaderResourceView(mResource, &srvDesc, handle.cpu);

    mSRVHandle = handle;
}

void PixelBuffer::CreateUAV(const DescriptorHandle &handle, uint32_t mipSlice) {
    if (!mResource) {
        return;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = mFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = mipSlice;

    gDevice->CreateUnorderedAccessView(mResource, nullptr, &uavDesc, handle.cpu);

    mUAVHandles[mipSlice] = handle;
}

}