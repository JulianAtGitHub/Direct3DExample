#include "stdafx.h"
#include "RenderTargetBuffer.h"

namespace Render {

RenderTargetBuffer::RenderTargetBuffer(ID3D12Resource *resource)
: PixelBuffer()
, mClearedColor(0.0f, 0.0f, 0.0f, 1.0f)
{
    Initialize(resource);
}

RenderTargetBuffer::RenderTargetBuffer(uint32_t width, uint32_t height, DXGI_FORMAT format)
: PixelBuffer()
, mClearedColor(0.0f, 0.0f, 0.0f, 1.0f)
{
    Initialize(width, height, format);
}

RenderTargetBuffer::~RenderTargetBuffer(void) {

}

void RenderTargetBuffer::Initialize(ID3D12Resource *resource) {
    if (!resource) {
        return;
    }

    mUsageState = D3D12_RESOURCE_STATE_PRESENT;
    mResource = resource;

    D3D12_RESOURCE_DESC desc = mResource->GetDesc();
    mWidth = static_cast<uint32_t>(desc.Width);
    mHeight = static_cast<uint32_t>(desc.Height);
    mPitch = mWidth * BytesPerPixel(desc.Format);
    mFormat = desc.Format;

    mViewDesc = {};

    // Setup output view to SRGB color space
    // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space
    //switch (mFormat) {
    //    case DXGI_FORMAT_R8G8B8A8_UNORM:
    //        mViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    //    case DXGI_FORMAT_B8G8R8A8_UNORM:
    //        mViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    //    default: 
    //        mViewDesc.Format = mFormat; 
    //        break;
    //}

    mViewDesc.Format = mFormat;
    mViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
}

void RenderTargetBuffer::Initialize(uint32_t width, uint32_t height, DXGI_FORMAT format) {
    mPitch = width * BytesPerPixel(format);
    mWidth = width;
    mHeight = height;

    mUsageState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    mFormat = format;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = format;
    clearValue.Color[0] = mClearedColor.x;
    clearValue.Color[1] = mClearedColor.y;
    clearValue.Color[2] = mClearedColor.z;
    clearValue.Color[3] = mClearedColor.w;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, mWidth, mHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    FillSampleDesc(resDesc.SampleDesc, format);
    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, mUsageState, &clearValue, IID_PPV_ARGS(&mResource)));

    mViewDesc = {};
    mViewDesc.Format = mFormat;
    mViewDesc.ViewDimension = (gMSAAState == DisableMSAA ? D3D12_RTV_DIMENSION_TEXTURE2D : D3D12_RTV_DIMENSION_TEXTURE2DMS);
}

void RenderTargetBuffer::CreateView(const DescriptorHandle &handle) {
    if (!mResource) {
        return;
    }

    gDevice->CreateRenderTargetView(mResource, &mViewDesc, handle.cpu);
    mSRVHandle = handle;
}

}
