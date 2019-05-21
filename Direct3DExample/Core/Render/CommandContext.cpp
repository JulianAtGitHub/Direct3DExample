#include "pch.h"
#include "CommandContext.h"
#include "CommandQueue.h"
#include "LinerAllocator.h"
#include "RenderCore.h"

namespace Render {

CommandContext::CommandContext(const D3D12_COMMAND_LIST_TYPE type)
: mType(type)
, mQueue(nullptr)
, mCommandList(nullptr)
, mCommandAlloctor(nullptr)
, mFenceValue(0)
, mCpuAllocator(nullptr)
, mGpuAllocator(nullptr)
, mResourceBarriers(MAX_RESOURCE_BARRIER)
{
    Initialize();
}

CommandContext::~CommandContext(void) {
    Destroy();
}

void CommandContext::Initialize(void) {
    mQueue = new CommandQueue(mType);

    mCpuAllocator = new LinerAllocator(LinerAllocator::CpuWritable);
    mGpuAllocator = new LinerAllocator(LinerAllocator::GpuExclusive);
}

void CommandContext::Destroy(void) {
    mCommandList->Close();

    mQueue->DiscardAllocator(mCommandAlloctor, mFenceValue);
    mCommandAlloctor = nullptr;
    DeleteAndSetNull(mQueue);

    ReleaseAndSetNull(mCommandList);

    DeleteAndSetNull(mCpuAllocator);
    DeleteAndSetNull(mGpuAllocator);
}

void CommandContext::Begin(void) {
    mCommandAlloctor = mQueue->QueryAllocator();
    if (mCommandList) {
        mCommandList->Reset(mCommandAlloctor, nullptr);
    } else {
        ASSERT_SUCCEEDED(gDevice->CreateCommandList(0, mType, mCommandAlloctor, nullptr, IID_PPV_ARGS(&mCommandList)));
    }
}

void CommandContext::End(bool flush) {
    if (!mCommandList) {
        return;
    }

    FlushResourceBarriers();

    uint64_t fenceValue = mQueue->ExecuteCommandList(mCommandList);
    mQueue->DiscardAllocator(mCommandAlloctor, fenceValue);
    mCommandAlloctor = nullptr;

    uint64_t completeValue = mQueue->UpdateCompleteFence();
    mCpuAllocator->CleanupPages(fenceValue, completeValue);
    mGpuAllocator->CleanupPages(fenceValue, completeValue);

    if (flush) {
        mQueue->WaitForFence(fenceValue);
    }
}

void CommandContext::AddResourceBarrier(D3D12_RESOURCE_BARRIER &barrier) {
    mResourceBarriers.PushBack(barrier);
    if (mResourceBarriers.Count() >= MAX_RESOURCE_BARRIER) {
        FlushResourceBarriers();
    }
}

void CommandContext::FlushResourceBarriers(void) {
    if (mResourceBarriers.Count() > 0) {
        mCommandList->ResourceBarrier(mResourceBarriers.Count(), mResourceBarriers.Data());
        mResourceBarriers.Clear();
    }

}

}