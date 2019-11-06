#include "stdafx.h"
#include "DescriptorHeap.h"
#include "RenderCore.h"

namespace Render {

DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
: mHeap(nullptr)
, mDescriptorSize(0)
, mTotalCount(0)
, mRemainingCount(0)
{
    Initialize(type, count);
}

DescriptorHeap::~DescriptorHeap(void) {
    Destroy();
}

void DescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count) {
    ASSERT_PRINT((count > 0));

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = count;
    heapDesc.Type = type;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 1;

    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    ASSERT_SUCCEEDED(gDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mHeap)));
    mDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(type);
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

    uint32_t offset = mTotalCount - mRemainingCount;
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