#pragma once

#include "PixelBuffer.h"

namespace Render {

class DepthStencilBuffer : public PixelBuffer {
public:
    DepthStencilBuffer(ID3D12Device *device, uint32_t width, uint32_t height);
    virtual ~DepthStencilBuffer(void);

    INLINE float GetClearedDepth(void) { return mClearedDepth; }
    INLINE uint8_t GetClearedStencil(void) { return mClearedStencil; }

    void Create(DXGI_FORMAT format, DescriptorHandle &handle);

private:
    float   mClearedDepth;
    uint8_t mClearedStencil;

};

}