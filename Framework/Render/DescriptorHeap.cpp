#include "stdafx.h"
#include "DescriptorHeap.h"
#include "RenderCore.h"

namespace Render {

DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
: mHeap(nullptr)
, mType(type)
, mDescriptorSize(0)
, mTotalCount(0)
, mRemainingCount(0)
{
    Initialize(count);
}

DescriptorHeap::~DescriptorHeap(void) {
    Destroy();
}

void DescriptorHeap::Initialize(uint32_t count) {
    ASSERT_PRINT((count > 0));

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = count;
    heapDesc.Type = mType;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 1;

    if (mType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        || mType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    ASSERT_SUCCEEDED(gDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mHeap)));
    mDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(mType);
    mTotalCount = count;
    mRemainingCount = count;
}

void DescriptorHeap::Destroy(void) {
    ReleaseAndSetNull(mHeap);
    mDescriptorSize = 0;
    mTotalCount = 0;
    mRemainingCount = 0;
}

DescriptorHandle DescriptorHeap::Allocate(void) {
    ASSERT_PRINT((mRemainingCount > 0));

    uint32_t offset = GetUsedCount();
    -- mRemainingCount;

    return DescriptorHandle(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + offset * mDescriptorSize,
                            mHeap->GetGPUDescriptorHandleForHeapStart().ptr + offset * mDescriptorSize);
}

DescriptorHandle DescriptorHeap::GetHandle(uint32_t index) {
    if (index >= mTotalCount) {
        return DescriptorHandle();
    }

    return DescriptorHandle(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + index * mDescriptorSize,
                            mHeap->GetGPUDescriptorHandleForHeapStart().ptr + index * mDescriptorSize);
}

}