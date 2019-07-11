#pragma once

#include "PixelBuffer.h"

namespace Render {

class DepthStencilBuffer : public PixelBuffer {
public:
    DepthStencilBuffer(uint32_t width, uint32_t height, DXGI_FORMAT format);
    virtual ~DepthStencilBuffer(void);

    INLINE float GetClearedDepth(void) { return mClearedDepth; }
    INLINE uint8_t GetClearedStencil(void) { return mClearedStencil; }

    void CreateView(DescriptorHandle &handle);

private:
    void Initialize(DXGI_FORMAT format);

    float   mClearedDepth;
    uint8_t mClearedStencil;

};

}