#pragma once

#include "PixelBuffer.h"

namespace Render {

class RenderTargetBuffer : public PixelBuffer {
public:
    RenderTargetBuffer(ID3D12Resource *resource);
    virtual ~RenderTargetBuffer(void);

    INLINE XMFLOAT4 GetClearedColor(void) { return mClearedColor; }
    INLINE float * GetClearedColorData(void) { return &mClearedColor.x; }

    void CreateView(const DescriptorHandle &handle);

private:
    void Initialize(ID3D12Resource *resource);

    XMFLOAT4   mClearedColor;
};

}