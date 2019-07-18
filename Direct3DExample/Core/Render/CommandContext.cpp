#include "pch.h"
#include "CommandContext.h"
#include "CommandQueue.h"
#include "RenderCore.h"
#include "LinerAllocator.h"
#include "Resource/PixelBuffer.h"
#include "Resource/RenderTargetBuffer.h"
#include "Resource/DepthStencilBuffer.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"

namespace Render {

ID3D12DescriptorHeap * CommandContext::msDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

CommandContext::CommandContext(const D3D12_COMMAND_LIST_TYPE type)
: mType(type)
, mQueue(nullptr)
, mCommandList(nullptr)
, mDXRCommandList(nullptr)
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
    mQueue->DiscardAllocator(mCommandAlloctor, mFenceValue);
    mCommandAlloctor = nullptr;
    DeleteAndSetNull(mQueue);

    ReleaseAndSetNull(mCommandList);
    ReleaseAndSetNull(mDXRCommandList);

    DeleteAndSetNull(mCpuAllocator);
    DeleteAndSetNull(mGpuAllocator);
}

void CommandContext::Begin(ID3D12PipelineState *pipeline) {
    mCommandAlloctor = mQueue->QueryAllocator();
    if (mCommandList) {
        mCommandList->Reset(mCommandAlloctor, pipeline);
    } else {
        ASSERT_SUCCEEDED(gDevice->CreateCommandList(1, mType, mCommandAlloctor, pipeline, IID_PPV_ARGS(&mCommandList)));
        if (gRayTracingSupport && gDXRDevice) {
            ASSERT_SUCCEEDED(mCommandList->QueryInterface(IID_PPV_ARGS(&mDXRCommandList)));
        }
    }
}

uint64_t CommandContext::End(bool waitUtilComplete) {
    if (!mCommandList) {
        return 0;
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

    return fenceValue;
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

    if (oldState != newState) {
        D3D12_RESOURCE_BARRIER barrierDesc = {};
        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc.Transition.pResource = resource->Get();
        barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc.Transition.StateBefore = oldState;
        barrierDesc.Transition.StateAfter = newState;
        barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        mCommandList->ResourceBarrier(1, &barrierDesc);
        resource->SetUsageState(newState);

    } else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
        InsertUAVBarrier(resource);
    }
}

void CommandContext::InsertUAVBarrier(GPUResource *resource) {
    if (!resource) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.UAV.pResource = resource->Get();
    mCommandList->ResourceBarrier(1, &barrierDesc);
}

void CommandContext::UploadBuffer(GPUResource *resource, size_t offset, const void *buffer, size_t size) {
    if (!resource || !buffer) {
        return;
    }

    LinerAllocator::MemoryBlock uploadMem = mCpuAllocator->Allocate(size);

    if (IsAligned(uploadMem.cpuAddress, 16) && IsAligned(buffer, 16)) {
        SIMDMemCopy(uploadMem.cpuAddress, buffer, DivideByMultiple(size, 16));
    } else {
        memcpy(uploadMem.cpuAddress, buffer, size);
    }

    TransitResource(resource, D3D12_RESOURCE_STATE_COPY_DEST);
    mCommandList->CopyBufferRegion(resource->Get(), offset, uploadMem.buffer->Get(), uploadMem.offset, size);
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
    UINT64 uploadSize = GetRequiredIntermediateSize(resource->Get(), 0, count);

    LinerAllocator::MemoryBlock uploadMem = mCpuAllocator->Allocate(uploadSize);
    UpdateSubresources(mCommandList, resource->Get(), uploadMem.buffer->Get(), uploadMem.offset, 0, count, subDatas);
    TransitResource(resource, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void CommandContext::ClearColor(RenderTargetBuffer *resource) {
    mCommandList->ClearRenderTargetView(resource->GetHandle().cpu, resource->GetClearedColorData(), 0, nullptr);
}

void CommandContext::ClearDepth(DepthStencilBuffer *resource) {
    mCommandList->ClearDepthStencilView(resource->GetHandle().cpu, D3D12_CLEAR_FLAG_DEPTH, resource->GetClearedDepth(), 0, 0, nullptr);
}

void CommandContext::ClearStencil(DepthStencilBuffer *resource) {
    mCommandList->ClearDepthStencilView(resource->GetHandle().cpu, D3D12_CLEAR_FLAG_STENCIL, 0.0f, resource->GetClearedStencil(), 0, nullptr);
}

void CommandContext::ClearDepthAndStencil(DepthStencilBuffer *resource) {
    mCommandList->ClearDepthStencilView(resource->GetHandle().cpu, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, resource->GetClearedDepth(), resource->GetClearedStencil(), 0, nullptr);
}

void CommandContext::SetRenderTarget(RenderTargetBuffer *renderTarget, DepthStencilBuffer *depthStencil) {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderTarget->GetHandle().cpu;
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil->GetHandle().cpu;
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void CommandContext::SetRootSignature(RootSignature *rootSignature) {
    ASSERT_PRINT(rootSignature->Get() != nullptr);
    mCommandList->SetGraphicsRootSignature(rootSignature->Get());
}

void CommandContext::SetDescriptorHeaps(DescriptorHeap **heaps, uint32_t count) {
    ASSERT_PRINT((heaps != nullptr && count <= D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES));
    for (uint32_t i = 0; i < count; ++i) {
        msDescriptorHeaps[i] = heaps[i]->Get();
    }
    mCommandList->SetDescriptorHeaps(count, msDescriptorHeaps);
}

void CommandContext::ExecuteBundle(ID3D12GraphicsCommandList *bundle) {
    mCommandList->ExecuteBundle(bundle);
}

}