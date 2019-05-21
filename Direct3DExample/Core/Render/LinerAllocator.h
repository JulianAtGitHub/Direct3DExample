#pragma once

#include "GPUResource.h"

namespace Render {

class LinerAllocator {
public:
    enum MemoryType {
        Invalid         =-1,
        GpuExclusive    = 0,
        CpuWritable     = 1,
        MemoryTypeMax   = 2
    };

    struct MemoryBlock {
        GPUResource    *buffer;
        size_t          size;
        void           *cpuAddress;
        UINT64          gpuAddress;
    };

    LinerAllocator(MemoryType type);
    ~LinerAllocator(void);

    MemoryBlock Allocate(size_t size);
    void CleanupPages(uint64_t fenceValue, uint64_t completeValue);

private:
    struct MemoryPage : public GPUResource {
        MemoryType  mType;
        size_t      mSize;
        size_t      mOffset;
        void       *mCPUAddress;

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

    };

    struct UsedPage {
        uint64_t    fenceValue;
        MemoryPage *page;
    };

    MemoryPage * AllocatePage(void);
    MemoryBlock AllocateLarge(size_t size);
    void DestoryPages(void);

    const static size_t MEMORY_ALIGNMENT        = 0x100;    // 256
    const static size_t MEMORY_ALIGNMENT_MASK   = 0xff;     // 255
    const static size_t GPU_MEMORY_PAGE_SIZE    = 0x10000;  // 64K
    const static size_t CPU_MEMORY_PAGE_SIZE    = 0x200000; // 2MB

    MemoryType              mType;
    size_t                  mPageSize;

    MemoryPage             *mCurrentPage;
    CList<MemoryPage *>     mPagePool;
    CList<MemoryPage *>     mUsingPages;
    CQueue<MemoryPage *>    mPendingPages;
    CQueue<UsedPage>        mUsedPages;

    CList<MemoryPage *>     mLargePages;
    CQueue<UsedPage>        mUsedLargePages;
};

}
