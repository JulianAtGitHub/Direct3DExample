#pragma once

#include "PixelBuffer.h"

namespace Render {

class DepthStencilBuffer : public PixelBuffer {
public:
    DepthStencilBuffer(ID3D12Device *device);
    virtual ~DepthStencilBuffer(void);

    INLINE float GetClearedDepth(void) { return mClearedDepth; }
    INLINE uint8_t GetClearedStencil(void) { return mClearedStencil; }

    void Create(uint32_t width, uint32_t height, DXGI_FORMAT format, DescriptorHandle &handle);

private:
    float   mClearedDepth;
    uint8_t mClearedStencil;

};

}