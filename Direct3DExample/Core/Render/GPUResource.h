#pragma once

namespace Render {

class GPUResource {
public:
    GPUResource(ID3D12Device *device);
    virtual ~GPUResource(void);

    INLINE ID3D12Resource * GetResource(void) { return mResource; }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(void) { return mGPUVirtualAddress; }

protected:
    void DestoryResource(void);

    ID3D12Device               *mDevice;
    ID3D12Resource             *mResource;
    D3D12_GPU_VIRTUAL_ADDRESS   mGPUVirtualAddress;
};

}