#pragma once

namespace Render {

class CommandQueue {
public:
    CommandQueue(ID3D12Device *device, const D3D12_COMMAND_LIST_TYPE type);
    ~CommandQueue(void);

    INLINE uint64_t GetLastCompleteValue(void) const { return mLastCompleteValue; }

    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFence(uint64_t fenceValue);
    void WaitForIdle(void);
    void StallForFence(ID3D12Fence *fence, uint64_t fenceValue);

    ID3D12CommandAllocator * QueryAllocator(void);
    void DiscardAllocator(ID3D12CommandAllocator *allocator, uint64_t fenceValue);
    uint64_t ExecuteCommandList(ID3D12CommandList* commandList);

private:
    void Initialize(void);
    void Destroy(void);

    uint64_t IncreaseFence(void);

    struct UsedAllocator {
        uint64_t                fenceValue;
        ID3D12CommandAllocator *allocator;
    };

    const static uint32_t           FENCE_VALUE_MASK = 56;

    const D3D12_COMMAND_LIST_TYPE   mType;
    ID3D12Device                   *mDevice;
    ID3D12CommandQueue             *mQueue;
    CList<ID3D12CommandAllocator *> mAllocatorPool;
    CQueue<UsedAllocator>           mUsedAllocators;

    ID3D12Fence                    *mFence;
    HANDLE                          mFenceEvent;
    uint64_t                        mNextFenceValue;
    uint64_t                        mLastCompleteValue;
};

}