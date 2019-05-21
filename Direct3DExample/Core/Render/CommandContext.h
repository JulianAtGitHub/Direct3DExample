#pragma once

namespace Render {

class CommandQueue;
class LinerAllocator;

class CommandContext {
public:
    CommandContext(const D3D12_COMMAND_LIST_TYPE type);
    ~CommandContext(void);

    void Begin(void);
    void End(bool flush = false);

private:
    void Initialize(void);
    void Destroy(void);

    void AddResourceBarrier(D3D12_RESOURCE_BARRIER &barrier);
    void FlushResourceBarriers(void);

    const static uint32_t MAX_RESOURCE_BARRIER = 16;

    const D3D12_COMMAND_LIST_TYPE   mType;
    CommandQueue                   *mQueue;
    ID3D12GraphicsCommandList      *mCommandList;
    ID3D12CommandAllocator         *mCommandAlloctor;
    uint64_t                        mFenceValue;
    LinerAllocator                 *mCpuAllocator;
    LinerAllocator                 *mGpuAllocator;
    CList<D3D12_RESOURCE_BARRIER>   mResourceBarriers;
};

}
