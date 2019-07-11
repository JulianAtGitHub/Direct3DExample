#include "pch.h"
#include "ConstantBuffer.h"
#include "Core/Render/RenderCore.h"

namespace Render {

ConstantBuffer::ConstantBuffer(uint32_t size, uint32_t count)
: GPUResource()
, mElementSize(AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
, mElementCount(count)
, mMappedBuffer(nullptr)
{
    Initialize();
}

ConstantBuffer::~ConstantBuffer(void) {
    if (mResource) {
        mResource->Unmap(0, nullptr);
    }
}

void ConstantBuffer::Initialize(void) {
    if (!mElementSize || !mElementCount) {
        return;
    }
    
    mUsageState = D3D12_RESOURCE_STATE_GENERIC_READ;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(mElementSize * mElementCount * FRAME_COUNT);
    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, mUsageState, nullptr, IID_PPV_ARGS(&mResource)));
    mResource->SetName(L"ConstantBuffer");

    FillGPUAddress();

    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ASSERT_SUCCEEDED(mResource->Map(0, &readRange, &mMappedBuffer));
}

void * ConstantBuffer::GetMappedBuffer(uint32_t index, uint32_t frameIndex) {
    ASSERT_PRINT((index < mElementCount));
    ASSERT_PRINT((frameIndex < FRAME_COUNT));

    return ((uint8_t *)mMappedBuffer) + mElementSize * (FRAME_COUNT * index + frameIndex);
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGPUAddress(uint32_t index, uint32_t frameIndex) {
    ASSERT_PRINT((index < mElementCount));
    ASSERT_PRINT((frameIndex < FRAME_COUNT));

    return mGPUVirtualAddress + mElementSize * (FRAME_COUNT * index + frameIndex);
}

}
