#pragma once

#include "CommandQueue.h"

namespace Render {

class LinerAllocator;
class GPUResource;
class PixelBuffer;
class RenderTargetBuffer;
class DepthStencilBuffer;
class RootSignature;
class DescriptorHeap;

class CommandContext {
public:
    CommandContext(const D3D12_COMMAND_LIST_TYPE type);
    ~CommandContext(void);

    INLINE CommandQueue * GetQueue(void) const { return mQueue; }
    INLINE ID3D12GraphicsCommandList * GetCommandList(void) const { return mCommandList; }
    INLINE ID3D12GraphicsCommandList4 * GetCommandList4(void) const { return mCommandList4; }

    void Begin(ID3D12PipelineState *pipeline = nullptr);
    uint64_t End(bool waitUtilComplete = false);

    void TransitResource(GPUResource *resource, D3D12_RESOURCE_STATES newState);
    void InsertUAVBarrier(GPUResource *resource);
    void UploadBuffer(GPUResource *resource, size_t offset, const void *buffer, size_t size);
    void UploadTexture(PixelBuffer *resource, const void *data);
    void UploadTexture(GPUResource *resource, D3D12_SUBRESOURCE_DATA *subDatas, uint32_t count);

    void ClearColor(RenderTargetBuffer *resource);
    void ClearDepth(DepthStencilBuffer *resource);
    void ClearStencil(DepthStencilBuffer *resource);
    void ClearDepthAndStencil(DepthStencilBuffer *resource);

    void SetRenderTarget(RenderTargetBuffer *renderTarget, DepthStencilBuffer *depthStencil);

    void SetViewport(const D3D12_VIEWPORT& viewport);
    void SetViewport(float x, float y, float w, float h, float minDepth = D3D12_MIN_DEPTH, float maxDepth = D3D12_MAX_DEPTH);
    void SetScissor(const D3D12_RECT& scissor);
    void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
    void SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissor);
    void SetViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

    void SetRootSignature(RootSignature *rootSignature);
    void SetDescriptorHeaps(DescriptorHeap **heaps, uint32_t count);

    void ExecuteBundle(ID3D12GraphicsCommandList *bundle);

private:
    void Initialize(void);
    void Destroy(void);

    const static uint32_t VALID_COMPUTE_QUEUE_RESOURCE_STATES = D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                                                              | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                                                              | D3D12_RESOURCE_STATE_COPY_DEST
                                                              | D3D12_RESOURCE_STATE_COPY_SOURCE;
    static ID3D12DescriptorHeap    *msDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    const D3D12_COMMAND_LIST_TYPE   mType;
    CommandQueue                   *mQueue;
    ID3D12GraphicsCommandList      *mCommandList;
    ID3D12GraphicsCommandList4     *mCommandList4;
    ID3D12CommandAllocator         *mCommandAlloctor;
    uint64_t                        mFenceValue;
    LinerAllocator                 *mCpuAllocator;
    LinerAllocator                 *mGpuAllocator;
};

INLINE void CommandContext::SetViewport(const D3D12_VIEWPORT& viewport) {
    mCommandList->RSSetViewports( 1, &viewport );
}

INLINE void CommandContext::SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth) {
    D3D12_VIEWPORT viewport = {x, y, w, h, minDepth, maxDepth};
    mCommandList->RSSetViewports( 1, &viewport );
}

INLINE void CommandContext::SetScissor(const D3D12_RECT& scissor){
    mCommandList->RSSetScissorRects( 1, &scissor );
}

INLINE void CommandContext::SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) {
    SetScissor(CD3DX12_RECT(left, top, right, bottom));
}

INLINE void CommandContext::SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissor) {
    mCommandList->RSSetViewports( 1, &viewport );
    mCommandList->RSSetScissorRects( 1, &scissor );
}

INLINE void CommandContext::SetViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    SetViewport((float)x, (float)y, (float)w, (float)h);
    SetScissor(x, y, x + w, y + h);
}

}
