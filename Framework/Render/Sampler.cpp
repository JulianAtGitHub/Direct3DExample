#include "stdafx.h"
#include "Sampler.h"
#include "RenderCore.h"

namespace Render {

Sampler::Sampler(void)
{
    mDesc = {};
    mDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    mDesc.MipLODBias = 0;
    mDesc.MaxAnisotropy = 0;
    mDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    mDesc.MinLOD = 0.0f;
    mDesc.MaxLOD = D3D12_FLOAT32_MAX;
    SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    SetBorderColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

Sampler::~Sampler(void) {

}

void Sampler::Create(const DescriptorHandle &handle) {
    gDevice->CreateSampler(&mDesc, handle.cpu);
    mHandle = handle;
}

}
