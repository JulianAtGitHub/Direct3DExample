#include "pch.h"
#include "CommandQueue.h"

namespace Render {

CommandQueue::CommandQueue(ID3D12Device *device, const D3D12_COMMAND_LIST_TYPE type)
: mType(type)
, mDevice(device)
, mQueue(nullptr)
, mFence(nullptr)
, mFenceEvent(INVALID_HANDLE_VALUE)
, mNextFenceValue(((uint64_t)type << FENCE_VALUE_MASK) | 1)
, mLastCompleteValue((uint64_t)type << FENCE_VALUE_MASK)
{
    Initialize();
}

CommandQueue::~CommandQueue(void) {
    Destroy();
}

void CommandQueue::Initialize(void) {

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.NodeMask = 0;
    ASSERT_SUCCEEDED(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mQueue)));
    mQueue->SetName(L"CommandQueue::mQueue");

    ASSERT_SUCCEEDED(mDevice->CreateFence(mLastCompleteValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFence->SetName(L"CommandQueue::mFence");
    mFence->Signal((uint64_t)mType << FENCE_VALUE_MASK);

    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    ASSERT_PRINT(mFenceEvent != INVALID_HANDLE_VALUE);
}

void CommandQueue::Destroy(void) {
    for (uint32_t i = 0; i < mAllocatorPool.Count(); ++i) {
        mAllocatorPool.At(i)->Release();
    }
    mAllocatorPool.Clear();
    mUsedAllocators.Clear();

    CloseHandle(mFenceEvent);
    mFenceEvent = INVALID_HANDLE_VALUE;
    ReleaseAndSetNull(mFence);
    ReleaseAndSetNull(mQueue);
}

ID3D12CommandAllocator * CommandQueue::QueryAllocator(void) {
    ID3D12CommandAllocator *allocator = nullptr;
    uint64_t fenceValue = mFence->GetCompletedValue();

    if (mUsedAllocators.Count() > 0) {
        UsedAllocator &usedAllocator = mUsedAllocators.Front();
        if (usedAllocator.fenceValue <= fenceValue) {
            allocator = usedAllocator.allocator;
            ASSERT_SUCCEEDED(allocator->Reset());
            mUsedAllocators.Pop();
        }
    }

    if (!allocator) {
        ASSERT_SUCCEEDED(mDevice->CreateCommandAllocator(mType, IID_PPV_ARGS(&allocator)));
        wchar_t allocatorName[64];
        swprintf(allocatorName, 64, L"CommandAllocator %u", mAllocatorPool.Count());
        allocator->SetName(allocatorName);
        mAllocatorPool.PushBack(allocator);
    }

    return allocator;
}

void CommandQueue::DiscardAllocator(ID3D12CommandAllocator *allocator, uint64_t fenceValue) {
    if (!allocator) {
        return;
    }

    UsedAllocator usedAlloctor = { fenceValue, allocator };
    mUsedAllocators.Push(usedAlloctor);
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* commandList) {
    ASSERT_SUCCEEDED(commandList->Close());
    ID3D12CommandList *commandLists[] = { commandList };
    mQueue->ExecuteCommandLists(1, commandLists);
    return IncreaseFence();
}

uint64_t CommandQueue::IncreaseFence(void) {
    ASSERT_SUCCEEDED(mQueue->Signal(mFence, mNextFenceValue));
    return mNextFenceValue ++;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue) {
    if (fenceValue > mLastCompleteValue) {
        UpdateCompleteFence();
    }

    return fenceValue <= mLastCompleteValue;
}

void CommandQueue::WaitForFence(uint64_t fenceValue) {
    if (IsFenceComplete(fenceValue)) {
        return;
    }

    mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
    mLastCompleteValue = fenceValue;
}

void CommandQueue::WaitForIdle(void) {
    WaitForFence(IncreaseFence());
}

void CommandQueue::StallForFence(ID3D12Fence *fence, uint64_t fenceValue) {
    if (!fence) {
        return;
    }

    mQueue->Wait(fence, fenceValue);
}

}

