#include "stdafx.h"
#include "RenderTargetBuffer.h"

namespace Render {

RenderTargetBuffer::RenderTargetBuffer(ID3D12Resource *resource)
: PixelBuffer()
, mClearedColor(0.0f, 0.0f, 0.0f, 1.0f)
{
    Initialize(resource);
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

    // Setup output view to SRGB color space
    // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space
    mViewDesc = {};
    switch (mFormat) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            mViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            mViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        default: 
            mViewDesc.Format = mFormat; 
            break;
    }
    mViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
}

void RenderTargetBuffer::CreateView(const DescriptorHandle &handle) {
    if (!mResource) {
        return;
    }

    gDevice->CreateRenderTargetView(mResource, &mViewDesc, handle.cpu);
    mSRVHandle = handle;
}

}
