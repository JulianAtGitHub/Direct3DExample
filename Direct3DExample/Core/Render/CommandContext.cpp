#include "pch.h"
#include "CommandContext.h"
#include "CommandQueue.h"

namespace Render {

CommandContext::CommandContext(ID3D12Device *device, const D3D12_COMMAND_LIST_TYPE type)
: mType(type)
, mDevice(device)
, mQueue(nullptr)
, mCommandList(nullptr)
, mCommandAlloctor(nullptr)
, mFenceValue(0)
{
    Initialize();
}

CommandContext::~CommandContext(void) {
    Destroy();
}

void CommandContext::Initialize(void) {
    mQueue = new CommandQueue(mDevice, mType);
    mCommandAlloctor = mQueue->QueryAllocator();
    ASSERT_SUCCEEDED(mDevice->CreateCommandList(0, mType, mCommandAlloctor, nullptr, IID_PPV_ARGS(&mCommandList)));
}

void CommandContext::Destroy(void) {
    mQueue->DiscardAllocator(mCommandAlloctor, mFenceValue);
    mCommandAlloctor = nullptr;
    DeleteAndSetNull(mQueue);

    ReleaseAndSetNull(mCommandList);
}

}