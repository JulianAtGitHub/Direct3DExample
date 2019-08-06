#pragma once

#include "DescriptorHeap.h"

namespace Render {

class DescriptorHeapPool {
public:
    const static uint32_t DEFAULT_HEAP_SIZE = 256;

    DescriptorHeapPool(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count = DEFAULT_HEAP_SIZE);
    ~DescriptorHeapPool(void);

    DescriptorHandle Allocate(void);

private:
    void DestroyAll(void);

    std::vector<ID3D12DescriptorHeap *> mHeapPool;
    ID3D12DescriptorHeap               *mHeap;
    D3D12_DESCRIPTOR_HEAP_DESC          mHeapDesc;
    uint32_t                            mDescriptorSize;
    uint32_t                            mRemainingCount;
};

}