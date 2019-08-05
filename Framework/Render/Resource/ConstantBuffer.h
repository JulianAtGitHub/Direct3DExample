#pragma once

#include "GPUResource.h"

namespace Render {

class ConstantBuffer : public GPUResource {
public:
    ConstantBuffer(uint32_t size, uint32_t count, size_t align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    virtual ~ConstantBuffer(void);

    void CopyData(const void *src, size_t size, uint32_t index, uint32_t frameIndex);

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(uint32_t index, uint32_t frameIndex);

private:
    void Initialize(void);

    void * GetMappedBuffer(uint32_t index, uint32_t frameIndex);

    uint32_t    mElementSize;
    uint32_t    mElementCount;
    void       *mMappedBuffer;
};

}
