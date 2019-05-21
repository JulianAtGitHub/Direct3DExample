#pragma once

namespace Render {

class GPUResource {
public:
    GPUResource(void);
    virtual ~GPUResource(void);

    INLINE ID3D12Resource * GetResource(void) { return mResource; }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(void) { return mGPUVirtualAddress; }
    void FillVirtualAddress(void);

protected:
    void DestoryResource(void);

    ID3D12Resource             *mResource;
    D3D12_GPU_VIRTUAL_ADDRESS   mGPUVirtualAddress;
};

INLINE void GPUResource::FillVirtualAddress(void) {
    if (mResource) {
        mGPUVirtualAddress = mResource->GetGPUVirtualAddress();
    }
}

}