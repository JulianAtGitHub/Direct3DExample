#include "pch.h"
#include "DepthStencilBuffer.h"

namespace Render {

DepthStencilBuffer::DepthStencilBuffer(ID3D12Device *device)
: PixelBuffer(device)
, mClearedDepth(1.0f)
, mClearedStencil(0)
{

}

DepthStencilBuffer::~DepthStencilBuffer(void) {

}

void DepthStencilBuffer::Create(uint32_t width, uint32_t height, DXGI_FORMAT format, DescriptorHandle &handle) {
    DestoryResource();

    mWidth = width;
    mHeight = height;
    mFormat = format;

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = mFormat;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = format;
    depthStencilClearValue.DepthStencil.Depth = mClearedDepth;
    depthStencilClearValue.DepthStencil.Stencil = mClearedStencil;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, mWidth, mHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ASSERT_SUCCEEDED(mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthStencilClearValue, IID_PPV_ARGS(&mResource)));

    mDevice->CreateDepthStencilView(mResource, &depthStencilDesc, handle.cpu);

    FillVirtualAddress();

    mHandle = handle;
}

}