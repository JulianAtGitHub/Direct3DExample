#pragma once

namespace Render {

class GPUResource {
public:
    GPUResource(void);
    virtual ~GPUResource(void);

    INLINE D3D12_RESOURCE_STATES GetUsageState(void) const { return mUsageState; }
    INLINE void SetUsageState(D3D12_RESOURCE_STATES newState) { mUsageState = newState; }

    INLINE ID3D12Resource * GetResource(void) const { return mResource; }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(void) const { return mGPUVirtualAddress; }
    void FillVirtualAddress(void);

protected:
    void DestoryResource(void);

    D3D12_RESOURCE_STATES       mUsageState;
    ID3D12Resource             *mResource;
    D3D12_GPU_VIRTUAL_ADDRESS   mGPUVirtualAddress;
};

INLINE void GPUResource::FillVirtualAddress(void) {
    if (mResource) {
        mGPUVirtualAddress = mResource->GetGPUVirtualAddress();
    }
}

}