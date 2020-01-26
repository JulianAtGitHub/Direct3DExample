#include "stdafx.h"
#include "BRDFIntegrationPass.h"
#include "BRDFIntegration.cs.h"

BRDFIntegrationPass::BRDFIntegrationPass(void) 
: mRootSignature(nullptr)
, mPipelineState(nullptr)
{
    Initialize();
}

BRDFIntegrationPass::~BRDFIntegrationPass(void) {
    Destroy();
}

void BRDFIntegrationPass::Initialize(void) {
    mRootSignature = new Render::RootSignature(Render::RootSignature::Compute, RootSigSlotMax);
    mRootSignature->SetConstants(ConstantSlot, 2, 0);
    mRootSignature->SetDescriptorTable(BRDFTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    mRootSignature->Create();

    mPipelineState = new Render::ComputeState();
    mPipelineState->SetShader(gscBRDFIntegration, sizeof(gscBRDFIntegration));
    mPipelineState->Create(mRootSignature);
}

void BRDFIntegrationPass::Destroy(void) {
    DeleteAndSetNull(mRootSignature);
    DeleteAndSetNull(mPipelineState);
}

void BRDFIntegrationPass::Dispatch(Render::PixelBuffer *brdfTex) {
    if (Render::gCommand->IsBegin()) {
        return;
    }

    if (!brdfTex) {
        return;
    }

    Render::DescriptorHeap *resourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    brdfTex->CreateUAV(resourceHeap->Allocate(), 0, false);

    D3D12_RESOURCE_STATES oldState = brdfTex->GetUsageState();

    Render::gCommand->Begin();

    Render::gCommand->TransitResource(brdfTex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Render::gCommand->SetPipelineState(mPipelineState);
    Render::gCommand->SetRootSignature(mRootSignature);

    Render::DescriptorHeap *heaps[] = { resourceHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));

    float constants[] = { 1.0f / brdfTex->GetWidth(), 1.0f / brdfTex->GetHeight() };
    Render::gCommand->SetComputeRootConstants(ConstantSlot, constants, _countof(constants));
    Render::gCommand->SetComputeRootDescriptorTable(BRDFTexSlot, resourceHeap->GetHandle(0));

    Render::gCommand->Dispatch2D(brdfTex->GetWidth(), brdfTex->GetHeight());

    Render::gCommand->InsertUAVBarrier(brdfTex);

    Render::gCommand->TransitResource(brdfTex, oldState);

    Render::gCommand->End(true);

    delete resourceHeap;
}
