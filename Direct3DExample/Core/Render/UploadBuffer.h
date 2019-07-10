#pragma once

#include "GPUResource.h"

namespace Render {

class UploadBuffer : public GPUResource {
public:
    UploadBuffer(uint32_t size);
    virtual ~UploadBuffer(void);

    void UploadData(const void *data, uint32_t size, uint32_t offset = 0);

private:
    void Initialize(void);

    uint32_t    mBufferSize;
    void       *mMappedBuffer;
};

}