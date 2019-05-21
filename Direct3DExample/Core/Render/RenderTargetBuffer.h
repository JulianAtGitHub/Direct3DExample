#pragma once

#include "PixelBuffer.h"

namespace Render {

class RenderTargetBuffer : public PixelBuffer {
public:
    RenderTargetBuffer(void);
    virtual ~RenderTargetBuffer(void);

    INLINE XMFLOAT4 GetClearedColor(void) { return mClearedColor; }

    void CreateFromSwapChain(ID3D12Resource *resource, DescriptorHandle &handle);

private:
    XMFLOAT4   mClearedColor;
};

}
