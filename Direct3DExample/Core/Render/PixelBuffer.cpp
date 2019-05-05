#include "pch.h"
#include "PixelBuffer.h"

namespace Render {

PixelBuffer::PixelBuffer(ID3D12Device *device, uint32_t width, uint32_t height)
: GPUResource(device)
, mWidth(width)
, mHeight(height)
, mFormat(DXGI_FORMAT_UNKNOWN)
{

}

PixelBuffer::~PixelBuffer(void) {

}

}