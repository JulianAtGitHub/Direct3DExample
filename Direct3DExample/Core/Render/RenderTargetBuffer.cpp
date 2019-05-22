#include "pch.h"
#include "RenderTargetBuffer.h"
#include "RenderCore.h"

namespace Render {

RenderTargetBuffer::RenderTargetBuffer(void)
: PixelBuffer()
, mClearedColor(0.0f, 0.0f, 0.0f, 1.0f)
{
    mUsageState = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

RenderTargetBuffer::~RenderTargetBuffer(void) {

}

void RenderTargetBuffer::CreateFromSwapChain(ID3D12Resource *resource, DescriptorHandle &handle) {
    if (!resource) {
        return;
    }

    DestoryResource();

    mResource = resource;
    gDevice->CreateRenderTargetView(mResource, nullptr, handle.cpu);

    FillGPUAddress();

    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    mWidth = static_cast<uint32_t>(desc.Width);
    mHeight = static_cast<uint32_t>(desc.Height);
    mFormat = desc.Format;
    mHandle = handle;
}

}
