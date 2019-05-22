#pragma once

namespace Render {

class CommandQueue;
class LinerAllocator;
class GPUResource;

class CommandContext {
public:
    CommandContext(const D3D12_COMMAND_LIST_TYPE type);
    ~CommandContext(void);

    INLINE CommandQueue * GetCommandQueue(void) { return mQueue; }

    void Begin(ID3D12PipelineState *pipeline = nullptr);
    void End(bool waitUtilComplete = false);

    void TransitResource(GPUResource *resource, D3D12_RESOURCE_STATES newState);
    void UploadBuffer(GPUResource *resource, size_t offset, void *buffer, size_t size);

private:
    void Initialize(void);
    void Destroy(void);

    const static uint32_t VALID_COMPUTE_QUEUE_RESOURCE_STATES = D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                                                              | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                                                              | D3D12_RESOURCE_STATE_COPY_DEST
                                                              | D3D12_RESOURCE_STATE_COPY_SOURCE;

    const D3D12_COMMAND_LIST_TYPE   mType;
    CommandQueue                   *mQueue;
    ID3D12GraphicsCommandList      *mCommandList;
    ID3D12CommandAllocator         *mCommandAlloctor;
    uint64_t                        mFenceValue;
    LinerAllocator                 *mCpuAllocator;
    LinerAllocator                 *mGpuAllocator;
};

}
