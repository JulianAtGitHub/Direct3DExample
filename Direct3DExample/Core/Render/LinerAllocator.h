#pragma once

#include "GPUResource.h"

namespace Render {

class LinerAllocator {
public:
    enum MemoryType {
        Invalid         = 0,
        GpuExclusive    = 1,
        CpuWritable     = 2
    };

    struct MemoryBlock {
        GPUResource    *buffer;
        size_t          size;
        size_t          offset;
        void           *cpuAddress;
        UINT64          gpuAddress;
    };

    LinerAllocator(MemoryType type);
    ~LinerAllocator(void);

    MemoryBlock Allocate(size_t size);
    void CleanupPages(uint64_t fenceValue, uint64_t completeValue);

private:
    class MemoryPage : public GPUResource {
    public:
        MemoryPage(MemoryType type, size_t size);
        virtual ~MemoryPage(void);

        INLINE size_t LeftSize(void) { return mSize - mOffset; }
        INLINE void * GetCurrentCPUAddress(void) { return (uint8_t*)(mCPUAddress) + mOffset; }
        INLINE UINT64 GetCurrentGPUAddress(void) { return mGPUVirtualAddress + mOffset; }

        INLINE void Map(void) { if (mResource && !mCPUAddress) { mResource->Map(0, nullptr, &mCPUAddress); } }
        INLINE void Unmap(void) { if (mResource && mCPUAddress) { mResource->Unmap(0, nullptr); mCPUAddress = nullptr; } }

    private:
        void Create(MemoryType type, size_t size);
        void Destory(void);

        MemoryType  mType;
        size_t      mSize;
        size_t      mOffset;
        void       *mCPUAddress;

        friend class LinerAllocator;
    };

    struct UsedPage {
        uint64_t    fenceValue;
        MemoryPage *page;
    };

    MemoryPage * AllocatePage(void);
    MemoryBlock AllocateLarge(size_t size);
    void DestoryPages(void);

    const static size_t MEMORY_ALIGNMENT        = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    const static size_t MEMORY_ALIGNMENT_MASK   = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1;
    const static size_t GPU_MEMORY_PAGE_SIZE    = 0x10000;  // 64K
    const static size_t CPU_MEMORY_PAGE_SIZE    = 0x200000; // 2MB

    MemoryType              mType;
    size_t                  mPageSize;

    MemoryPage             *mCurrentPage;
    CList<MemoryPage *>     mPagePool;      // all pages
    CList<MemoryPage *>     mUsingPages;    // pages are using in current render loop
    CQueue<MemoryPage *>    mPendingPages;  // pagea are created and ready for re-use
    CQueue<UsedPage>        mUsedPages;     // pages are using in previous render loop which is not finished

    CQueue<UsedPage>        mUsedLargePages;// pages are ready to destroy
    CList<MemoryPage *>     mLargePages;    // pages are using in current render loop
};

}
