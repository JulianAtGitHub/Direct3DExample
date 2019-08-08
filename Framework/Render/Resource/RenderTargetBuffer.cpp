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
}

void RenderTargetBuffer::CreateView(const DescriptorHandle &handle) {
    if (!mResource) {
        return;
    }

    gDevice->CreateRenderTargetView(mResource, nullptr, handle.cpu);
    mHandle = handle;
}

}
