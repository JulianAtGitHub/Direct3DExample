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

void CommandContext::Begin(ID3D12PipelineState *pipeline) {
    mCommandAlloctor = mQueue->QueryAllocator();
    if (mCommandList) {
        mCommandList->Reset(mCommandAlloctor, pipeline);
    } else {
        ASSERT_SUCCEEDED(gDevice->CreateCommandList(0, mType, mCommandAlloctor, pipeline, IID_PPV_ARGS(&mCommandList)));
    }
}

void CommandContext::End(bool waitUtilComplete) {
    if (!mCommandList) {
        return;
    }

    uint64_t fenceValue = mQueue->ExecuteCommandList(mCommandList);
    mQueue->DiscardAllocator(mCommandAlloctor, fenceValue);
    mCommandAlloctor = nullptr;

    uint64_t completeValue = mQueue->UpdateCompleteFence();
    mCpuAllocator->CleanupPages(fenceValue, completeValue);
    mGpuAllocator->CleanupPages(fenceValue, completeValue);

    if (waitUtilComplete) {
        mQueue->WaitForFence(fenceValue);
    }
}

void CommandContext::TransitResource(GPUResource *resource, D3D12_RESOURCE_STATES newState) {
    D3D12_RESOURCE_STATES oldState = resource->GetUsageState();

    if (mType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        ASSERT_PRINT((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState);
        ASSERT_PRINT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState);
    }

    D3D12_RESOURCE_BARRIER BarrierDesc = {};
    if (oldState != newState) {
        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = resource->GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = oldState;
        BarrierDesc.Transition.StateAfter = newState;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        resource->SetUsageState(newState);
    }
    else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        BarrierDesc.UAV.pResource = resource->GetResource();
    }

    mCommandList->ResourceBarrier(1, &BarrierDesc);
}

}