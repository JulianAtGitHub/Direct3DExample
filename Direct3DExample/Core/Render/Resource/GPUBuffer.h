#pragma once

#include "GPUResource.h"
#include "Core/Render/DescriptorHeap.h"

namespace Render {

class GPUBuffer : public GPUResource {
public:
    GPUBuffer(size_t size, D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
    virtual ~GPUBuffer(void);

    INLINE size_t GetBufferSize(void) const { return mBufferSize; }
    INLINE const DescriptorHandle & GetHandle(void) const { return mHandle; }

    D3D12_VERTEX_BUFFER_VIEW FillVertexBufferView(size_t offset, uint32_t size, uint32_t stride);
    D3D12_INDEX_BUFFER_VIEW FillIndexBufferView(size_t offset, uint32_t size, bool is16Bit = true);

    void CreateVertexBufferSRV(DescriptorHandle &handle, uint32_t count, uint32_t size);
    void CreateIndexBufferSRV(DescriptorHandle &handle, uint32_t count);

private:
    void Initialize(void);

    size_t                  mBufferSize;
    D3D12_RESOURCE_FLAGS    mFlag;
    DescriptorHandle        mHandle;
};

INLINE D3D12_VERTEX_BUFFER_VIEW GPUBuffer::FillVertexBufferView(size_t offset, uint32_t size, uint32_t stride) {
    return { mGPUVirtualAddress + offset, size, stride };
}

INLINE D3D12_INDEX_BUFFER_VIEW GPUBuffer::FillIndexBufferView(size_t offset, uint32_t size, bool is16Bit) {
    return { mGPUVirtualAddress + offset, size, is16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT };
}

}