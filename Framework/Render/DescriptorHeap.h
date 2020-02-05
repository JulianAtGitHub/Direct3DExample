#pragma once

namespace Render {

class DescriptorHeap {
public:
    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    ~DescriptorHeap(void);

    DescriptorHandle Allocate(void);
    DescriptorHandle GetHandle(uint32_t index);

    INLINE ID3D12DescriptorHeap * Get(void) const { return mHeap; }
    INLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType(void) const { return mType; }
    INLINE uint32_t GetDescriptorSize(void) const { return mDescriptorSize; }
    INLINE uint32_t GetUsedCount(void) { return mTotalCount - mRemainingCount; }
    INLINE uint32_t GetRemainingCount(void) const { return mRemainingCount; }

private:
    void Initialize(uint32_t count);
    void Destroy(void);

    ID3D12DescriptorHeap       *mHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE  mType;
    uint32_t                    mDescriptorSize;
    uint32_t                    mTotalCount;
    uint32_t                    mRemainingCount;
};

}