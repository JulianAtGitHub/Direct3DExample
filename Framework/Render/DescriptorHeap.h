#pragma once

namespace Render {

class DescriptorHeap {
public:
    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    ~DescriptorHeap(void);

    DescriptorHandle Allocate(void);

    INLINE ID3D12DescriptorHeap * Get(void) const { return mHeap; }
    INLINE uint32_t GetRemainingCount(void) const { return mRemainingCount; }

private:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    void Destroy(void);

    ID3D12DescriptorHeap   *mHeap;
    uint32_t                mDescriptorSize;
    uint32_t                mTotalCount;
    uint32_t                mRemainingCount;
};

}