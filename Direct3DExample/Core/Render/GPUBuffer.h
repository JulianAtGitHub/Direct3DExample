#pragma once

#include "GPUResource.h"

namespace Render {

class GPUBuffer : public GPUResource {
public:
    GPUBuffer(uint32_t size, uint32_t count);
    virtual ~GPUBuffer(void);

    INLINE size_t GetBufferSize(void) const { return mBufferSize; }
    INLINE uint32_t GetElementSize(void) const { return mElementSize; }
    INLINE uint32_t GetElementCount(void) const { return mElementCount; }

private:
    void Initialize(void);

    size_t      mBufferSize;
    uint32_t    mElementSize;
    uint32_t    mElementCount;
};

}