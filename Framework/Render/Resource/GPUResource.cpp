#include "stdafx.h"
#include "GPUResource.h"

namespace Render {

GPUResource::GPUResource(void)
: mUsageState(D3D12_RESOURCE_STATE_COMMON)
, mResource(nullptr)
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