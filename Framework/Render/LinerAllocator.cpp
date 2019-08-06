#include "stdafx.h"
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
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

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

    FillGPUAddress();

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
    mLargePages.push_back(page);
    return { page, size, page->mOffset, page->mCPUAddress, page->GetGPUAddress() };
}

LinerAllocator::MemoryPage * LinerAllocator::AllocatePage(void) {
    MemoryPage *page = nullptr;
    if (mPendingPages.size() > 0) {
        page = mPendingPages.front();
        mPendingPages.pop();
        page->mOffset = 0;
    } else {
        page = new MemoryPage(mType, mPageSize);
        mPagePool.push_back(page);
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
        mUsingPages.push_back(mCurrentPage);
        mCurrentPage = nullptr;
    }

    if (!mCurrentPage) {
        mCurrentPage = AllocatePage();
    }

    MemoryBlock block = { mCurrentPage , alignedSize, mCurrentPage->mOffset, mCurrentPage->GetCurrentCPUAddress(), mCurrentPage->GetCurrentGPUAddress() };
    mCurrentPage->mOffset = AlignUp(mCurrentPage->mOffset + alignedSize, MEMORY_ALIGNMENT);

    return block;
}

void LinerAllocator::CleanupPages(uint64_t fenceValue, uint64_t completeValue) {
 
    while (mUsedPages.size() > 0 && mUsedPages.front().fenceValue <= completeValue) {
        mPendingPages.push(mUsedPages.front().page);
        mUsedPages.pop();
    }

    mUsingPages.push_back(mCurrentPage);
    mCurrentPage = nullptr;

    for (auto usingPage : mUsingPages) {
        mUsedPages.push({fenceValue , usingPage});
    }
    mUsingPages.clear();

    // handle large page
    while (mUsedLargePages.size() && mUsedLargePages.front().fenceValue <= completeValue) {
        delete mUsedLargePages.front().page;
        mUsedLargePages.pop();
    }

    for (auto page : mLargePages) {
        page->Unmap();
        mUsedLargePages.push({ fenceValue , page });
    }
    mLargePages.clear();
}

void LinerAllocator::DestoryPages(void) {
    for (auto page : mPagePool) {
        delete page;
    }
    mPagePool.clear();
    mCurrentPage = nullptr;
    mUsingPages.clear();

    while (!mPendingPages.empty()) { mPendingPages.pop(); }
    while (!mUsedPages.empty()) { mUsedPages.pop(); }

    while (!mUsedLargePages.empty()) {
        delete mUsedLargePages.front().page;
        mUsedLargePages.pop();
    }
    for (auto page : mLargePages) {
        delete page;
    }
    mLargePages.clear();
}

}
