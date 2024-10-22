#include "stdafx.h"
#include "CommandQueue.h"
#include "RenderCore.h"

namespace Render {

CommandQueue::CommandQueue(const D3D12_COMMAND_LIST_TYPE type)
: mType(type)
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
    queueDesc.Type = mType;
    queueDesc.NodeMask = 1;
    ASSERT_SUCCEEDED(gDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mQueue)));
    mQueue->SetName(L"CommandQueue::mQueue");

    ASSERT_SUCCEEDED(gDevice->CreateFence(mLastCompleteValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFence->SetName(L"CommandQueue::mFence");
    mFence->Signal((uint64_t)mType << FENCE_VALUE_MASK);

    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    ASSERT_PRINT(mFenceEvent != INVALID_HANDLE_VALUE);
}

void CommandQueue::Destroy(void) {
    for (auto allocator : mAllocatorPool) {
        allocator->Release();
    }
    mAllocatorPool.clear();
    while(!mUsedAllocators.empty()) { mUsedAllocators.pop(); }

    CloseHandle(mFenceEvent);
    mFenceEvent = INVALID_HANDLE_VALUE;
    ReleaseAndSetNull(mFence);
    ReleaseAndSetNull(mQueue);
}

ID3D12CommandAllocator * CommandQueue::QueryAllocator(void) {
    ID3D12CommandAllocator *allocator = nullptr;
    uint64_t fenceValue = mFence->GetCompletedValue();

    if (mUsedAllocators.size() > 0) {
        UsedAllocator &usedAllocator = mUsedAllocators.front();
        if (usedAllocator.fenceValue <= fenceValue) {
            allocator = usedAllocator.allocator;
            ASSERT_SUCCEEDED(allocator->Reset());
            mUsedAllocators.pop();
        }
    }

    if (!allocator) {
        ASSERT_SUCCEEDED(gDevice->CreateCommandAllocator(mType, IID_PPV_ARGS(&allocator)));
        wchar_t allocatorName[64];
        swprintf(allocatorName, 64, L"CommandAllocator %llu", mAllocatorPool.size());
        allocator->SetName(allocatorName);
        mAllocatorPool.push_back(allocator);
    }

    return allocator;
}

void CommandQueue::DiscardAllocator(ID3D12CommandAllocator *allocator, uint64_t fenceValue) {
    if (!allocator) {
        return;
    }

    UsedAllocator usedAlloctor = { fenceValue, allocator };
    mUsedAllocators.push(usedAlloctor);
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

