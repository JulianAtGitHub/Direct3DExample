#pragma once

#include "GPUResource.h"

namespace Render {

class UploadBuffer : public GPUResource {
public:
    UploadBuffer(size_t size);
    virtual ~UploadBuffer(void);

    INLINE size_t GetBufferSize(void) const { return mBufferSize; }

    void UploadData(const void *data, size_t size, uint32_t offset = 0);

private:
    void Initialize(void);

    size_t  mBufferSize;
    void   *mMappedBuffer;
};

}