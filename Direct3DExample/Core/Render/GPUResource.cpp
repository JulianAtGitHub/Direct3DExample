#include "pch.h"
#include "GPUResource.h"

namespace Render {

GPUResource::GPUResource(ID3D12Device *device)
: mDevice(device)
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