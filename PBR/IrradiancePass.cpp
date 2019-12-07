#include "stdafx.h"
#include "IrradiancePass.h"
#include "Irradiance.cs.h"

IrradiancePass::IrradiancePass(void) 
: mRootSignature(nullptr)
, mPipelineState(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
{
    Initialize();
}

IrradiancePass::~IrradiancePass(void) {
    Destroy();
}

void IrradiancePass::Initialize(void) {
    mRootSignature = new Render::RootSignature(Render::RootSignature::Compute, RootSigSlotMax);
    mRootSignature->SetConstants(ConstantSlot, 2, 0);
    mRootSignature->SetDescriptorTable(EnvTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    mRootSignature->SetDescriptorTable(SamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mRootSignature->SetDescriptorTable(IrrTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    mRootSignature->Create();

    mPipelineState = new Render::ComputeState();
    mPipelineState->SetShader(gscIrradiance, sizeof(gscIrradiance));
    mPipelineState->Create(mRootSignature);

    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    mSampler = new Render::Sampler();
    mSampler->SetFilter(D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    mSampler->SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    mSampler->Create(mSamplerHeap->Allocate());
}

void IrradiancePass::Destroy(void) {
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mRootSignature);
    DeleteAndSetNull(mPipelineState);
}

void IrradiancePass::Dispatch(Render::PixelBuffer *envTex, Render::PixelBuffer *irrTex) {
    if (Render::gCommand->IsBegin()) {
        return;
    }

    if (!envTex || !irrTex) {
        return;
    }

    Render::DescriptorHeap *resourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);
    envTex->CreateSRV(resourceHeap->Allocate(), false);
    irrTex->CreateUAV(resourceHeap->Allocate(), 0, false);

    D3D12_RESOURCE_STATES oldState = irrTex->GetUsageState();

    Render::gCommand->Begin();

    Render::gCommand->TransitResource(irrTex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Render::gCommand->SetPipelineState(mPipelineState);
    Render::gCommand->SetRootSignature(mRootSignature);

    Render::DescriptorHeap *heaps[] = { resourceHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));

    XMFLOAT2 constant(1.0f / irrTex->GetWidth(), 1.0f / irrTex->GetHeight());
    Render::gCommand->SetComputeRootConstants(ConstantSlot, &constant, 2);
    Render::gCommand->SetComputeRootDescriptorTable(EnvTexSlot, resourceHeap->GetHandle(0));
    Render::gCommand->SetComputeRootDescriptorTable(SamplerSlot, mSampler->GetHandle());
    Render::gCommand->SetComputeRootDescriptorTable(IrrTexSlot, resourceHeap->GetHandle(1));

    Render::gCommand->Dispatch2D(irrTex->GetWidth(), irrTex->GetHeight());

    Render::gCommand->InsertUAVBarrier(irrTex);

    Render::gCommand->TransitResource(irrTex, oldState);

    Render::gCommand->End(true);

    delete resourceHeap;
}

