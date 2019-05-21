#include "pch.h"
#include "GPUResource.h"

namespace Render {

GPUResource::GPUResource(void)
: mResource(nullptr)
, mGPUVirtualAddress(0)
{

}

GPUResource::~GPUResource(void) {
    DestoryResource();
}

void GPUResource::DestoryResource(void) {
    ReleaseAndSetNull(mResource);
}

}