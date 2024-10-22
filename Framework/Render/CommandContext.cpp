#include "stdafx.h"
#include "CommandContext.h"
#include "CommandQueue.h"
#include "RenderCore.h"
#include "LinerAllocator.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "PipelineState.h"
#include "AccelerationStructure.h"
#include "RayTracingState.h"
#include "Resource/GPUBuffer.h"
#include "Resource/PixelBuffer.h"
#include "Resource/RenderTargetBuffer.h"
#include "Resource/DepthStencilBuffer.h"

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
, mIsBegin(false)
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

void CommandContext::Begin(PipelineState *pipeline) {
    ASSERT_PRINT(!mIsBegin);
    if (mIsBegin) {
        return;
    }

    ID3D12PipelineState *pipelineState = pipeline ? pipeline->GetPipelineState() : nullptr;
    mCommandAlloctor = mQueue->QueryAllocator();
    if (mCommandList) {
        mCommandList->Reset(mCommandAlloctor, pipelineState);
    } else {
        ASSERT_SUCCEEDED(gDevice->CreateCommandList(1, mType, mCommandAlloctor, pipelineState, IID_PPV_ARGS(&mCommandList)));
        if (gRayTracingSupport && gDXRDevice) {
            ASSERT_SUCCEEDED(mCommandList->QueryInterface(IID_PPV_ARGS(&mDXRCommandList)));
        }
    }

    mIsBegin = true;
}

uint64_t CommandContext::End(bool waitUtilComplete) {
    ASSERT_PRINT(mIsBegin);
    if (!mIsBegin) {
        return 0;
    }

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

    mIsBegin = false;

    return fenceValue;
}

void CommandContext::SetPipelineState(PipelineState *pipeline) {
    mCommandList->SetPipelineState(pipeline ? pipeline->GetPipelineState() : nullptr);
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
    texRes.RowPitch = resource->GetPitch();
    texRes.SlicePitch = texRes.RowPitch * resource->GetHeight();

    UploadTexture(resource, &texRes, 1);
}

void CommandContext::UploadTexture(GPUResource *resource, D3D12_SUBRESOURCE_DATA *subDatas, uint32_t count) {
    TransitResource(resource, D3D12_RESOURCE_STATE_COPY_DEST);

    UINT64 uploadSize = GetRequiredIntermediateSize(resource->Get(), 0, count);
    LinerAllocator::MemoryBlock uploadMem = mCpuAllocator->Allocate(uploadSize);
    UpdateSubresources(mCommandList, resource->Get(), uploadMem.buffer->Get(), uploadMem.offset, 0, count, subDatas);

    TransitResource(resource, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void CommandContext::CopyResource(GPUResource *dest, GPUResource *src) {
    if (!dest || !dest->Get() || !src || !src->Get()) {
        return;
    }

    TransitResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);

    mCommandList->CopyResource(dest->Get(), src->Get());
}

void CommandContext::ClearColor(RenderTargetBuffer *resource) {
    mCommandList->ClearRenderTargetView(resource->GetSRVHandle().cpu, resource->GetClearedColorData(), 0, nullptr);
}

void CommandContext::ClearDepth(DepthStencilBuffer *resource) {
    mCommandList->ClearDepthStencilView(resource->GetSRVHandle().cpu, D3D12_CLEAR_FLAG_DEPTH, resource->GetClearedDepth(), 0, 0, nullptr);
}

void CommandContext::ClearStencil(DepthStencilBuffer *resource) {
    mCommandList->ClearDepthStencilView(resource->GetSRVHandle().cpu, D3D12_CLEAR_FLAG_STENCIL, 0.0f, resource->GetClearedStencil(), 0, nullptr);
}

void CommandContext::ClearDepthAndStencil(DepthStencilBuffer *resource) {
    mCommandList->ClearDepthStencilView(resource->GetSRVHandle().cpu, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, resource->GetClearedDepth(), resource->GetClearedStencil(), 0, nullptr);
}

void CommandContext::SetRenderTarget(RenderTargetBuffer *renderTarget, DepthStencilBuffer *depthStencil) {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderTarget->GetSRVHandle().cpu;
    if (depthStencil) {
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil->GetSRVHandle().cpu;
        mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    } else {
        mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    }
}

void CommandContext::SetRootSignature(RootSignature *rootSignature) {
    ASSERT_PRINT(rootSignature->Get() != nullptr);
    if (rootSignature->GetType() == RootSignature::Graphics) {
        mCommandList->SetGraphicsRootSignature(rootSignature->Get());
    } else {
        mCommandList->SetComputeRootSignature(rootSignature->Get());
    }
}

void CommandContext::SetGraphicsRootShaderResourceView(uint32_t index, GPUResource *resource) {
    ASSERT_PRINT(resource->Get() != nullptr);
    mCommandList->SetGraphicsRootShaderResourceView(index, resource->GetGPUAddress());
}

void CommandContext::SetComputeRootShaderResourceView(uint32_t index, GPUResource *resource) {
    ASSERT_PRINT(resource->Get() != nullptr);
    mCommandList->SetComputeRootShaderResourceView(index, resource->GetGPUAddress());
}

void CommandContext::SetDescriptorHeaps(DescriptorHeap **heaps, uint32_t count) {
    ASSERT_PRINT((heaps != nullptr && count <= D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES));
    for (uint32_t i = 0; i < count; ++i) {
        msDescriptorHeaps[i] = heaps[i]->Get();
    }
    mCommandList->SetDescriptorHeaps(count, msDescriptorHeaps);
}

void CommandContext::ResolveSubresource(RenderTargetBuffer *dest, RenderTargetBuffer *source, DXGI_FORMAT format) {
    mCommandList->ResolveSubresource(dest->Get(), 0, source->Get(), 0, format);
}

void CommandContext::BuildAccelerationStructure(AccelerationStructure *blas) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.Inputs = blas->GetInputs();
    desc.ScratchAccelerationStructureData = blas->GetScratch()->GetGPUAddress();
    desc.DestAccelerationStructureData = blas->GetResult()->GetGPUAddress();

    mDXRCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
    InsertUAVBarrier(blas->GetResult());
}

void CommandContext::SetRayTracingState(RayTracingState *state) {
    ASSERT_PRINT(state->Get() != nullptr);
    mDXRCommandList->SetPipelineState1(state->Get());
}

void CommandContext::DispatchRay(RayTracingState *state, uint32_t width, uint32_t height, uint32_t depth) {
    ASSERT_PRINT(state->Get() != nullptr);
    D3D12_DISPATCH_RAYS_DESC &rayDesc = state->GetRayDescriptor();
    rayDesc.Width = width;
    rayDesc.Height = height;
    rayDesc.Depth = depth;
    mDXRCommandList->DispatchRays(&rayDesc);
}

}