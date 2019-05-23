#include "pch.h"
#include "CommandContext.h"
#include "CommandQueue.h"
#include "LinerAllocator.h"
#include "PixelBuffer.h"
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
    if (!resource) {
        return;
    }

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

void CommandContext::UploadBuffer(GPUResource *resource, size_t offset, const void *buffer, size_t size) {
    if (!resource || !buffer) {
        return;
    }

    LinerAllocator::MemoryBlock uploadMem = mCpuAllocator->Allocate(size);

    SIMDMemCopy(uploadMem.cpuAddress, buffer, DivideByMultiple(size, 16));

    TransitResource(resource, D3D12_RESOURCE_STATE_COPY_DEST);
    mCommandList->CopyBufferRegion(resource->GetResource(), offset, uploadMem.buffer->GetResource(), uploadMem.offset, size);
    TransitResource(resource, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void CommandContext::UploadTexture(PixelBuffer *resource, const void *data) {
    if (!resource || !data) {
        return;
    }

    D3D12_SUBRESOURCE_DATA texRes;
    texRes.pData = data;
    texRes.RowPitch = resource->GetPitch() * (BitsPerPixel(resource->GetFormat()) >> 3);
    texRes.SlicePitch = texRes.RowPitch * resource->GetHeight();

    UploadTexture(resource, &texRes, 1);
}

void CommandContext::UploadTexture(GPUResource *resource, D3D12_SUBRESOURCE_DATA *subDatas, uint32_t count) {
    UINT64 uploadSize = GetRequiredIntermediateSize(resource->GetResource(), 0, count);

    LinerAllocator::MemoryBlock uploadMem = mCpuAllocator->Allocate(uploadSize);
    UpdateSubresources(mCommandList, resource->GetResource(), uploadMem.buffer->GetResource(), uploadMem.offset, 0, count, subDatas);
    TransitResource(resource, D3D12_RESOURCE_STATE_GENERIC_READ);
}

}