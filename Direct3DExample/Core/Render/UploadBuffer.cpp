#include "pch.h"
#include "UploadBuffer.h"
#include "RenderCore.h"

namespace Render {

UploadBuffer::UploadBuffer(uint32_t size)
: GPUResource()
, mBufferSize(AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
, mMappedBuffer(nullptr)
{
    Initialize();
}

UploadBuffer::~UploadBuffer(void) {
    if (mResource) {
        mResource->Unmap(0, nullptr);
    }
}

void UploadBuffer::Initialize(void) {
    if (!mBufferSize) {
        return;
    }

    mUsageState = D3D12_RESOURCE_STATE_GENERIC_READ;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);
    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, mUsageState, nullptr, IID_PPV_ARGS(&mResource)));
    mResource->SetName(L"UploadBuffer");

    FillGPUAddress();

    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ASSERT_SUCCEEDED(mResource->Map(0, &readRange, &mMappedBuffer));
}

void UploadBuffer::UploadData(const void *data, uint32_t size, uint32_t offset) {
    if (!data) {
        return;
    }

    if (offset + size > mBufferSize) {
        size = mBufferSize - offset;
    }

    uint8_t *dest = (uint8_t *)mMappedBuffer;
    memcpy(dest + offset, data, size);
}

}