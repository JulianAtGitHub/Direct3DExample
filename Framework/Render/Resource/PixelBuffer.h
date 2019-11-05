#pragma once

#include "GPUResource.h"

namespace Render {

class PixelBuffer: public GPUResource {
public:
    PixelBuffer(uint32_t pitch, uint32_t width, uint32_t height, uint32_t mipLevels, DXGI_FORMAT format, 
                D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_COMMON, 
                D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
    PixelBuffer(void);
    virtual ~PixelBuffer(void);

    INLINE uint32_t GetPitch(void) const { return mPitch; }
    INLINE uint32_t GetWidth(void) const { return mWidth; }
    INLINE uint32_t GetHeight(void) const { return mHeight; }
    INLINE uint32_t GetMipLevels(void) const { return mMipLevels; }
    INLINE DXGI_FORMAT GetFormat(void) const { return mFormat; }
    INLINE const DescriptorHandle & GetSRVHandle(void) const { return mSRVHandle; }
    INLINE const DescriptorHandle & GetUAVHandle(uint32_t mipSlice = 0) const { return mUAVHandles[mipSlice]; }

    void CreateSRV(const DescriptorHandle &handle);
    void CreateUAV(const DescriptorHandle &handle, uint32_t mipSlice = 0);

protected:
    void Initialize(void);

    uint32_t                        mPitch; // bytes per row
    uint32_t                        mWidth;
    uint32_t                        mHeight;
    uint32_t                        mMipLevels;
    DXGI_FORMAT                     mFormat;
    D3D12_RESOURCE_FLAGS            mFlag;
    DescriptorHandle                mSRVHandle;
    std::vector<DescriptorHandle>   mUAVHandles;
};

}