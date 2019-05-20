#include "pch.h"
#include "DescriptorHeap.h"

namespace Render {

DescriptorHeap::DescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
: mDevice(device)
, mHeap(nullptr)
, mDescriptorSize(0)
, mRemainingCount(0)
{
    assert(count > 0);

    mHeapDesc = {};
    mHeapDesc.NumDescriptors = count;
    mHeapDesc.Type = type;
    mHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    mHeapDesc.NodeMask = 0;
}

DescriptorHeap::~DescriptorHeap(void) {
    DestroyAll();
}

void DescriptorHeap::DestroyAll(void) {
    for (uint32_t i = 0; i < mHeapPool.Count(); ++i) {
        mHeapPool.At(i)->Release();
    }

    mHeap = nullptr;
    mRemainingCount = 0;
}

DescriptorHandle DescriptorHeap::Allocate(void) {
    if (mRemainingCount == 0) {
        ASSERT_SUCCEEDED(mDevice->CreateDescriptorHeap(&mHeapDesc, IID_PPV_ARGS(&mHeap)));
        mDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(mHeapDesc.Type);
        mRemainingCount = mHeapDesc.NumDescriptors;
        mHeapPool.PushBack(mHeap);
    }

    uint32_t offset = mHeapDesc.NumDescriptors - mRemainingCount;
    DescriptorHandle handle(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + offset * mDescriptorSize,
                            mHeap->GetGPUDescriptorHandleForHeapStart().ptr + offset * mDescriptorSize);

    -- mRemainingCount;

    return handle;
}

}