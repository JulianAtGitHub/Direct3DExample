#include "pch.h"
#include "LinerAllocator.h"
#include "RenderCore.h"

namespace Render {

LinerAllocator::MemoryPage::MemoryPage(MemoryType type, size_t size)
: GPUResource()
, mType(Invalid)
, mSize(0)
, mOffset(0)
, mCPUAddress(nullptr)
{
    Create(type, size);
}

LinerAllocator::MemoryPage::~MemoryPage(void) {
    Destory();
}

void LinerAllocator::MemoryPage::Create(MemoryType type, size_t size) {

    mType = type;
    mSize = size;
    ASSERT_PRINT((mType == GpuExclusive || mType == CpuWritable));
    ASSERT_PRINT((size > 0));

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 0;
    heapProps.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC resDesc;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Alignment = 0;
    resDesc.Width = mSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (mType == GpuExclusive) {
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        mUsageState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    } else {
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        mUsageState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    ASSERT_SUCCEEDED(gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, mUsageState, nullptr, IID_PPV_ARGS(&mResource)));
    mResource->SetName(L"LinerAllocator Page");

    FillVirtualAddress();

    Map();
}

void LinerAllocator::MemoryPage::Destory(void) {
    Unmap();
    DestoryResource();

    mCPUAddress = nullptr;
    mType = Invalid;
    mSize = 0;
    mOffset = 0;
}

LinerAllocator::LinerAllocator(MemoryType type) 
: mType(type)
, mPageSize(type == GpuExclusive ? GPU_MEMORY_PAGE_SIZE : CPU_MEMORY_PAGE_SIZE)
, mCurrentPage(nullptr)
{

}

LinerAllocator::~LinerAllocator(void) {
    DestoryPages();
}

LinerAllocator::MemoryBlock LinerAllocator::AllocateLarge(size_t size) {
    ASSERT_PRINT((size > 0));
    MemoryPage *page = new MemoryPage(mType, size);
    mLargePages.PushBack(page);
    return { page, size, page->mCPUAddress, page->GetGPUAddress() };
}

LinerAllocator::MemoryPage * LinerAllocator::AllocatePage(void) {
    MemoryPage *page = nullptr;
    if (mPendingPages.Count() > 0) {
        page = mPendingPages.Front();
        mPendingPages.Pop();
        page->mOffset = 0;
    } else {
        page = new MemoryPage(mType, mPageSize);
        mPagePool.PushBack(page);
    }

    return page;
}

LinerAllocator::MemoryBlock LinerAllocator::Allocate(size_t size) {
    // allocated size
    const size_t alignedSize = AlignUp(size, MEMORY_ALIGNMENT);

    if (alignedSize > mPageSize) {
        return AllocateLarge(alignedSize);
    }

    if (mCurrentPage && mCurrentPage->LeftSize() < alignedSize) {
        mUsingPages.PushBack(mCurrentPage);
        mCurrentPage = nullptr;
    }

    if (!mCurrentPage) {
        mCurrentPage = AllocatePage();
    }

    MemoryBlock block = { mCurrentPage , alignedSize, mCurrentPage->GetCurrentCPUAddress(), mCurrentPage->GetCurrentGPUAddress() };
    mCurrentPage->mOffset = AlignUp(mCurrentPage->mOffset + alignedSize, MEMORY_ALIGNMENT);

    return block;
}

void LinerAllocator::CleanupPages(uint64_t fenceValue, uint64_t completeValue) {
 
    while (mUsedPages.Count() > 0 && mUsedPages.Front().fenceValue <= completeValue) {
        mPendingPages.Push(mUsedPages.Front().page);
        mUsedPages.Pop();
    }

    mUsingPages.PushBack(mCurrentPage);
    mCurrentPage = nullptr;

    for (uint32_t i = 0; i < mUsingPages.Count(); ++i) {
        mUsedPages.Push({fenceValue , mUsingPages.At(i)});
    }
    mUsingPages.Clear();

    // handle large page
    while (mUsedLargePages.Count() && mUsedLargePages.Front().fenceValue <= completeValue) {
        delete mUsedLargePages.Front().page;
        mUsedLargePages.Pop();
    }

    for (uint32_t i = 0; i < mLargePages.Count(); ++i) {
        MemoryPage *page = mLargePages.At(i);
        page->Unmap();
        mUsedLargePages.Push({ fenceValue , page });
    }
    mLargePages.Clear();
}

void LinerAllocator::DestoryPages(void) {
    for (uint32_t i = 0; i < mPagePool.Count(); ++i) {
        delete mPagePool.At(i);
    }
    mPagePool.Clear();
    mCurrentPage = nullptr;
    mUsingPages.Clear();
    mPendingPages.Clear();
    mUsedPages.Clear();

    while (mUsedLargePages.Count()) {
        delete mUsedLargePages.Front().page;
        mUsedLargePages.Pop();
    }
    for (uint32_t i = 0; i < mLargePages.Count(); ++i) {
        delete mLargePages.At(i);
    }
    mLargePages.Clear();
}

}
