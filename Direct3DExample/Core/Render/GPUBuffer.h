#pragma once

#include "GPUResource.h"

namespace Render {

class GPUBuffer : public GPUResource {
public:
    GPUBuffer(uint32_t size, D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
    virtual ~GPUBuffer(void);

    INLINE size_t GetBufferSize(void) const { return mBufferSize; }

    D3D12_VERTEX_BUFFER_VIEW FillVertexBufferView(size_t offset, uint32_t size, uint32_t stride);
    D3D12_INDEX_BUFFER_VIEW FillIndexBufferView(size_t offset, uint32_t size, bool is16Bit = true);

private:
    void Initialize(void);

    size_t                  mBufferSize;
    D3D12_RESOURCE_FLAGS    mFlags;
};

INLINE D3D12_VERTEX_BUFFER_VIEW GPUBuffer::FillVertexBufferView(size_t offset, uint32_t size, uint32_t stride) {
    return { mGPUVirtualAddress + offset, size, stride };
}

INLINE D3D12_INDEX_BUFFER_VIEW GPUBuffer::FillIndexBufferView(size_t offset, uint32_t size, bool is16Bit) {
    return { mGPUVirtualAddress + offset, size, is16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT };
}

}