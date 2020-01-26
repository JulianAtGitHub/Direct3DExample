#include "stdafx.h"
#include "PrefilteredEnvPass.h"
#include "PrefilteredEnv.cs.h"

PrefilteredEnvPass::PrefilteredEnvPass(void)
: mRootSignature(nullptr)
, mPipelineState(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
{
    Initialize();
}

PrefilteredEnvPass::~PrefilteredEnvPass(void) {
    Destroy();
}

void PrefilteredEnvPass::Initialize(void) {
    mRootSignature = new Render::RootSignature(Render::RootSignature::Compute, RootSigSlotMax);
    mRootSignature->SetConstants(ConstantSlot, 3, 0);
    mRootSignature->SetDescriptorTable(EnvTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    mRootSignature->SetDescriptorTable(SamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mRootSignature->SetDescriptorTable(BlurredTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    mRootSignature->Create();

    mPipelineState = new Render::ComputeState();
    mPipelineState->SetShader(gscPrefilteredEnv, sizeof(gscPrefilteredEnv));
    mPipelineState->Create(mRootSignature);

    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    mSampler = new Render::Sampler();
    mSampler->SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    mSampler->Create(mSamplerHeap->Allocate());
}

void PrefilteredEnvPass::Destroy(void) {
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mRootSignature);
    DeleteAndSetNull(mPipelineState);
}

void PrefilteredEnvPass::Dispatch(Render::PixelBuffer *envTex, Render::PixelBuffer *blurredTex) {
    if (Render::gCommand->IsBegin()) {
        return;
    }

    if (!envTex || !blurredTex) {
        return;
    }

    uint32_t mipLevels = blurredTex->GetMipLevels();

    Render::DescriptorHeap *resourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1 + mipLevels);
    envTex->CreateSRV(resourceHeap->Allocate(), false);
    for (uint32_t i = 0; i < mipLevels; ++i) {
        blurredTex->CreateUAV(resourceHeap->Allocate(), i, false);
    }

    D3D12_RESOURCE_STATES oldState = blurredTex->GetUsageState();

    Render::gCommand->Begin();

    Render::gCommand->TransitResource(blurredTex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Render::gCommand->SetPipelineState(mPipelineState);
    Render::gCommand->SetRootSignature(mRootSignature);

    Render::DescriptorHeap *heaps[] = { resourceHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));

    Render::gCommand->SetComputeRootDescriptorTable(EnvTexSlot, resourceHeap->GetHandle(0));
    Render::gCommand->SetComputeRootDescriptorTable(SamplerSlot, mSampler->GetHandle());

    uint32_t texWidth = blurredTex->GetWidth();
    uint32_t texHeight = blurredTex->GetHeight();
    for (uint32_t i = 0; i < mipLevels; ++i) {
        uint32_t width = texWidth >> i;
        uint32_t height = texHeight >> i;
        float roughness = float(i) / (mipLevels  - 1);
        float constants[] = { 1.0f / width, 1.0f / height, roughness };
        Render::gCommand->SetComputeRootConstants(ConstantSlot, constants, _countof(constants));
        Render::gCommand->SetComputeRootDescriptorTable(BlurredTexSlot, resourceHeap->GetHandle(1 + i));

        Render::gCommand->Dispatch2D(width, height);

        Render::gCommand->InsertUAVBarrier(blurredTex);
    }

    Render::gCommand->TransitResource(blurredTex, oldState);

    Render::gCommand->End(true);

    delete resourceHeap;
}
