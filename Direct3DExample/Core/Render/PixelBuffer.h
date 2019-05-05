#pragma once

#include "GPUResource.h"
#include "DescriptorHeap.h"

namespace Render {

class PixelBuffer: public GPUResource {
public:
    PixelBuffer(ID3D12Device *device, uint32_t width = 0, uint32_t height = 0);
    virtual ~PixelBuffer(void);

    INLINE uint32_t GetWidth(void) { return mWidth; }
    INLINE uint32_t GetHeight(void) { return mHeight; }
    INLINE DXGI_FORMAT GetFormat(void) { return mFormat; }
    INLINE const DescriptorHandle & GetHandle(void) const { return mHandle; }

protected:
    uint32_t            mWidth;
    uint32_t            mHeight;
    DXGI_FORMAT         mFormat;
    DescriptorHandle    mHandle;
};

}