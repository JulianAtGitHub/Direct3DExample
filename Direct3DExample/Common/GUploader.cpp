#include "pch.h"
#include "GUploader.h"

GUploader::GUploader(ID3D12Device *device) {
    Initialize(device);
}

GUploader::~GUploader(void) {
    CloseHandle(mFenceEvent);
    ReleaseIfNotNull(mFence);

    ReleaseIfNotNull(mCommandList);
    ReleaseIfNotNull(mCommandAllocator);
    ReleaseIfNotNull(mCommandQueue);
}

void GUploader::Initialize(ID3D12Device *device) {
    mDevice = device;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));

    ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, nullptr, IID_PPV_ARGS(&mCommandList)));

    ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    mFenceValue = 1;
}

void GUploader::WaitGPU(void) {
    const uint64_t fenceValue = mFenceValue;
    ThrowIfFailed(mCommandQueue->Signal(mFence, fenceValue));
    ++ mFenceValue;

    if (mFence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(fenceValue, mFenceEvent));
        WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
    }
}

