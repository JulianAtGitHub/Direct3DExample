#pragma once

#include "GPUResource.h"
#include "DescriptorHeap.h"

namespace Render {

class PixelBuffer: public GPUResource {
public:
    PixelBuffer(void);
    PixelBuffer(uint32_t pitch, uint32_t width, uint32_t height, DXGI_FORMAT format);
    virtual ~PixelBuffer(void);

    INLINE uint32_t GetPitch(void) const { return mPitch; }
    INLINE uint32_t GetWidth(void) const { return mWidth; }
    INLINE uint32_t GetHeight(void) const { return mHeight; }
    INLINE DXGI_FORMAT GetFormat(void) const { return mFormat; }
    INLINE const DescriptorHandle & GetHandle(void) const { return mHandle; }

    void CreateSRView(DescriptorHandle &handle);

protected:
    void Initialize(void);

    uint32_t            mPitch; // bytes per row
    uint32_t            mWidth;
    uint32_t            mHeight;
    DXGI_FORMAT         mFormat;
    DescriptorHandle    mHandle;
};

}