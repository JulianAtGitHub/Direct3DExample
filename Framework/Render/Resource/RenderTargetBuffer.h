#pragma once

#include "PixelBuffer.h"

namespace Render {

class RenderTargetBuffer : public PixelBuffer {
public:
    RenderTargetBuffer(ID3D12Resource *resource);
    RenderTargetBuffer(uint32_t width, uint32_t height, DXGI_FORMAT format);
    virtual ~RenderTargetBuffer(void);

    INLINE XMFLOAT4 GetClearedColor(void) { return mClearedColor; }
    INLINE float * GetClearedColorData(void) { return &mClearedColor.x; }

    void CreateView(const DescriptorHandle &handle);

private:
    void Initialize(ID3D12Resource *resource);
    void Initialize(uint32_t width, uint32_t height, DXGI_FORMAT format);

    D3D12_RENDER_TARGET_VIEW_DESC   mViewDesc;
    XMFLOAT4                        mClearedColor;
};

}
