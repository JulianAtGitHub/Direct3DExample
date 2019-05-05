#pragma once

#include "PixelBuffer.h"

namespace Render {

class ColorBuffer : public PixelBuffer {
public:
    ColorBuffer(ID3D12Device *device);
    virtual ~ColorBuffer(void);

    INLINE XMFLOAT4 GetClearedColor(void) { return mClearedColor; }

    void CreateFromSwapChain(ID3D12Resource *resource, DescriptorHandle &handle);

private:
    XMFLOAT4   mClearedColor;
};

}
