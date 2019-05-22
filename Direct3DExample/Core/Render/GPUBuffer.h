#pragma once

#include "GPUResource.h"

namespace Render {

class GPUBuffer : public GPUResource {
public:
    GPUBuffer(void);
    virtual ~GPUBuffer(void);

    INLINE size_t GetBufferSize(void) const { return mBufferSize; }
    INLINE uint32_t GetElementSize(void) const { return mElementSize; }
    INLINE uint32_t GetElementCount(void) const { return mElementCount; }

    void Create(uint32_t size, uint32_t count);

private:
    size_t      mBufferSize;
    uint32_t    mElementSize;
    uint32_t    mElementCount;
};

}