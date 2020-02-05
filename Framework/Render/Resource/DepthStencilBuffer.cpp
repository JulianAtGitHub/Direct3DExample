#include "stdafx.h"
#include "DepthStencilBuffer.h"

namespace Render {

DepthStencilBuffer::DepthStencilBuffer(uint32_t width, uint32_t height, DXGI_FORMAT format)
: PixelBuffer()
, mClearedDepth(1.0f)
, mClearedStencil(0)
{
    mPitch = width * BytesPerPixel(format);
    mWidth = width;
    mHeight = height;
    Initialize(format);
}

DepthStencilBuffer::~DepthStencilBuffer(void) {

}

void DepthStencilBuffer::Initialize(DXGI_FORMAT format) {
    mUsageState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    mFormat = format;

    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = format;
    depthStencilClearValue.DepthStencil.Depth = mClearedDepth;
    depthStencilClearValue.DepthStencil.Stencil = mClearedStencil;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, mWidth, mHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    FillSampleDesc(resDesc.SampleDesc, format);
    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, mUsageState, &depthStencilClearValue, IID_PPV_ARGS(&mResource)));
}

void DepthStencilBuffer::CreateView(const DescriptorHandle &handle) {
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = mFormat;
    depthStencilDesc.ViewDimension = (gMSAAState == DisableMSAA ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DMS);
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    gDevice->CreateDepthStencilView(mResource, &depthStencilDesc, handle.cpu);
    mSRVHandle = handle;
}

}