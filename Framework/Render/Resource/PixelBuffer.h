#pragma once

#include "GPUResource.h"

namespace Render {

class PixelBuffer: public GPUResource {
public:
    PixelBuffer(uint32_t pitch, uint32_t width, uint32_t height, DXGI_FORMAT format, 
                D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_COMMON, 
                D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
    PixelBuffer(void);
    virtual ~PixelBuffer(void);

    INLINE uint32_t GetPitch(void) const { return mPitch; }
    INLINE uint32_t GetWidth(void) const { return mWidth; }
    INLINE uint32_t GetHeight(void) const { return mHeight; }
    INLINE DXGI_FORMAT GetFormat(void) const { return mFormat; }
    INLINE const DescriptorHandle & GetHandle(void) const { return mHandle; }

    void CreateSRV(DescriptorHandle &handle);
    void CreateUAV(DescriptorHandle &handle);

protected:
    void Initialize(void);

    uint32_t                mPitch; // bytes per row
    uint32_t                mWidth;
    uint32_t                mHeight;
    DXGI_FORMAT             mFormat;
    D3D12_RESOURCE_FLAGS    mFlag;
    DescriptorHandle        mHandle;
};

}