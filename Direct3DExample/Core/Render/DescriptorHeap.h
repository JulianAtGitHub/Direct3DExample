#pragma once

namespace Render {

struct DescriptorHandle {
    DescriptorHandle(SIZE_T cpuPtr = 0, UINT64 gpuPtr = 0) {
        cpu.ptr = cpuPtr;
        gpu.ptr = gpuPtr;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

class DescriptorHeap {
public:

    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    ~DescriptorHeap(void);

    DescriptorHandle Allocate(void);

private:
    void DestroyAll(void);

    CList<ID3D12DescriptorHeap *>   mHeapPool;
    ID3D12DescriptorHeap           *mHeap;
    D3D12_DESCRIPTOR_HEAP_DESC      mHeapDesc;
    uint32_t                        mDescriptorSize;
    uint32_t                        mRemainingCount;
};

}