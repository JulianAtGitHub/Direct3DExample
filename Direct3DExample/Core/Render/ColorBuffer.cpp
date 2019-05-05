#include "pch.h"
#include "ColorBuffer.h"

namespace Render {

ColorBuffer::ColorBuffer(ID3D12Device *device)
: PixelBuffer(device)
, mClearedColor(0.0f, 0.0f, 0.0f, 1.0f)
{

}

ColorBuffer::~ColorBuffer(void) {

}

void ColorBuffer::CreateFromSwapChain(ID3D12Resource *resource, DescriptorHandle &handle) {
    if (!resource) {
        return;
    }

    mResource = resource;
    mGPUVirtualAddress = resource->GetGPUVirtualAddress();
    mDevice->CreateRenderTargetView(mResource, nullptr, handle.cpu);

    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    mWidth = static_cast<uint32_t>(desc.Width);
    mHeight = static_cast<uint32_t>(desc.Height);
    mFormat = desc.Format;
    mHandle = handle;
}

}
