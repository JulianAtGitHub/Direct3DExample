#pragma once

namespace Render {

class CommandQueue;

class CommandContext {
public:
    CommandContext(ID3D12Device *device, const D3D12_COMMAND_LIST_TYPE type);
    ~CommandContext(void);

private:
    void Initialize(void);
    void Destroy(void);

    const D3D12_COMMAND_LIST_TYPE   mType;
    ID3D12Device                   *mDevice;
    CommandQueue                   *mQueue;
    ID3D12CommandList              *mCommandList;
    ID3D12CommandAllocator         *mCommandAlloctor;
    uint64_t                        mFenceValue;
};

}
