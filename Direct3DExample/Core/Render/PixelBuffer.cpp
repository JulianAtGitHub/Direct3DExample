#include "pch.h"
#include "PixelBuffer.h"

namespace Render {

PixelBuffer::PixelBuffer(uint32_t width, uint32_t height)
: GPUResource()
, mWidth(width)
, mHeight(height)
, mFormat(DXGI_FORMAT_UNKNOWN)
{

}

PixelBuffer::~PixelBuffer(void) {

}

}