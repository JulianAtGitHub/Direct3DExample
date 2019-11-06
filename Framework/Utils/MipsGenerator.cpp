#include "stdafx.h"
#include "Render/RenderCore.h"
#include "Render/RootSignature.h"
#include "Render/PipelineState.h"
#include "Render/DescriptorHeap.h"
#include "Render/Sampler.h"
#include "Render/CommandContext.h"
#include "Render/Resource/PixelBuffer.h"
#include "MipsGenerator.h"

#include "GenerateMipsLinear.h"
#include "GenerateMipsLinearOdd.h"
#include "GenerateMipsLinearOddX.h"
#include "GenerateMipsLinearOddy.h"
#include "GenerateMipsSRGB.h"
#include "GenerateMipsSRGBOdd.h"
#include "GenerateMipsSRGBOddX.h"
#include "GenerateMipsSRGBOddY.h"

namespace Utils {

MipsGenerator::MipsGenerator(void)
: mRootSignature(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
{
    Initialize();
}

MipsGenerator::~MipsGenerator(void) {
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mRootSignature);

    for (uint32_t i = 0; i < ColorSpaceMax; ++i) {
        for (uint32_t j = 0; j < SizeTypeMax; ++j) {
            DeleteAndSetNull(mPipelineState[i][j]);
        }
    }
}

void MipsGenerator::Initialize(void) {
    mRootSignature = new Render::RootSignature(Render::RootSignature::Compute, 4);
    mRootSignature->SetConstants(0, 4, 0);
    mRootSignature->SetDescriptorTable(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    mRootSignature->SetDescriptorTable(2, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mRootSignature->SetDescriptorTable(3, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0);
    mRootSignature->Create();

    for (uint32_t i = 0; i < ColorSpaceMax; ++i) {
        for (uint32_t j = 0; j < SizeTypeMax; ++j) {
            mPipelineState[i][j] = new Render::ComputeState();
        }
    }

    mPipelineState[LinearSpace][X_EVEN_Y_EVEN]->SetShader(gscGenerateMipsLinear, sizeof(gscGenerateMipsLinear));
    mPipelineState[LinearSpace][X_ODD_Y_EVEN]->SetShader(gscGenerateMipsLinearOddX, sizeof(gscGenerateMipsLinearOddX));
    mPipelineState[LinearSpace][X_EVEN_Y_ODD]->SetShader(gscGenerateMipsLinearOddY, sizeof(gscGenerateMipsLinearOddY));
    mPipelineState[LinearSpace][X_ODD_Y_ODD]->SetShader(gscGenerateMipsLinearOdd, sizeof(gscGenerateMipsLinearOdd));
    mPipelineState[SRGBSpace][X_EVEN_Y_EVEN]->SetShader(gscGenerateMipsSRGB, sizeof(gscGenerateMipsSRGB));
    mPipelineState[SRGBSpace][X_ODD_Y_EVEN]->SetShader(gscGenerateMipsSRGBOddX, sizeof(gscGenerateMipsSRGBOddX));
    mPipelineState[SRGBSpace][X_EVEN_Y_ODD]->SetShader(gscGenerateMipsSRGBOddY, sizeof(gscGenerateMipsSRGBOddY));
    mPipelineState[SRGBSpace][X_ODD_Y_ODD]->SetShader(gscGenerateMipsSRGBOdd, sizeof(gscGenerateMipsSRGBOdd));

    for (uint32_t i = 0; i < ColorSpaceMax; ++i) {
        for (uint32_t j = 0; j < SizeTypeMax; ++j) {
            mPipelineState[i][j]->Create(mRootSignature);
        }
    }

    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    mSampler = new Render::Sampler();
    mSampler->SetFilter(D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    mSampler->SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    mSampler->Create(mSamplerHeap->Allocate());
}

void MipsGenerator::Dispatch(Render::PixelBuffer *resource) {
    if (!resource) {
        return;
    }

    uint32_t mipLevels = resource->GetMipLevels();
    if (mipLevels <= 1) {
        return;
    }

    Render::DescriptorHeap *resourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mipLevels);
    resource->CreateSRV(resourceHeap->Allocate(), false);
    for (uint32_t i = 1; i < mipLevels; ++i) {
        resource->CreateUAV(resourceHeap->Allocate(), i, false);
    }

    D3D12_RESOURCE_STATES oldState = resource->GetUsageState();

    Render::gCommand->Begin();

    Render::gCommand->TransitResource(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Render::gCommand->SetRootSignature(mRootSignature);

    Render::DescriptorHeap *heaps[] = { resourceHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));

    Render::gCommand->SetComputeRootDescriptorTable(1, resourceHeap->GetHandle(0));
    Render::gCommand->SetComputeRootDescriptorTable(2, mSampler->GetHandle());

    ColorSpace space = Render::IsSRGB(resource->GetFormat()) ? SRGBSpace : LinearSpace;
    uint32_t mipsCount = mipLevels - 1;
    for (uint32_t topMip = 0; topMip < mipsCount; )
    {
        uint32_t srcWidth = resource->GetWidth() >> topMip;
        uint32_t srcHeight = resource->GetHeight() >> topMip;
        uint32_t dstWidth = srcWidth >> 1;
        uint32_t dstHeight = srcHeight >> 1;

        uint32_t sizeType = (srcWidth & 1) | ((srcHeight & 1) << 1);
        Render::gCommand->SetPipelineState(mPipelineState[space][sizeType]);
        
        uint32_t additionalMips;
        _BitScanForward((unsigned long*)&additionalMips, (dstWidth == 1 ? dstHeight : dstWidth) | (dstHeight == 1 ? dstWidth : dstHeight));
        uint32_t newMips = 1 + (additionalMips > 3 ? 3 : additionalMips);
        if (topMip + newMips > mipsCount) {
            newMips = mipsCount - topMip;
        }

        if (dstWidth == 0) {
            dstWidth = 1;
        }
        if (dstHeight == 0) {
            dstHeight = 1;
        }

        uint32_t constants[] = {topMip, newMips, 0, 0};
        *(float *)(constants + 2) = 1.0f / dstWidth;
        *(float *)(constants + 3) = 1.0f / dstHeight;
        Render::gCommand->SetComputeRootConstants(0, constants, 4);
        Render::gCommand->SetComputeRootDescriptorTable(3, resourceHeap->GetHandle(1 + topMip));

        Render::gCommand->Dispatch2D(dstWidth, dstHeight);

        Render::gCommand->InsertUAVBarrier(resource);

        topMip += newMips;
    }

    Render::gCommand->TransitResource(resource, oldState);

    Render::gCommand->End(true);

    delete resourceHeap;
}

}
