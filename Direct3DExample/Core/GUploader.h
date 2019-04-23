#pragma once

class GUploader {
public:
    GUploader(ID3D12Device *device);
    ~GUploader(void);

private:
    void Initialize(ID3D12Device *device);

    void WaitGPU(void);

    ID3D12Device *mDevice;
    ID3D12CommandQueue *mCommandQueue;
    ID3D12CommandAllocator *mCommandAllocator;
    ID3D12CommandList *mCommandList;

    ID3D12Fence *mFence;
    HANDLE mFenceEvent;
    uint64_t mFenceValue;
};
