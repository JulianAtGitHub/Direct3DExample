#pragma once

#include "DescriptorHeap.h"

namespace Render {

class Sampler {
public:
    Sampler(void);
    ~Sampler(void);

    INLINE D3D12_SAMPLER_DESC & GetDesc(void) { return mDesc; }
    INLINE DescriptorHandle & GetHandle(void) { return mHandle; }

    void SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE mode);
    void SetBorderColor(const XMFLOAT4 &color);

    void Create(DescriptorHandle &handle);

private:
    D3D12_SAMPLER_DESC  mDesc;
    DescriptorHandle    mHandle;

};

INLINE void Sampler::SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE mode) {
    mDesc.AddressU = mode;
    mDesc.AddressV = mode;
    mDesc.AddressW = mode;
}

INLINE void Sampler::SetBorderColor(const XMFLOAT4 &color) {
    mDesc.BorderColor[0] = color.x;
    mDesc.BorderColor[1] = color.y;
    mDesc.BorderColor[2] = color.z;
    mDesc.BorderColor[3] = color.w;
}

}