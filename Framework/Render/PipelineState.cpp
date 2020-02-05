#include "stdafx.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "RenderCore.h"

namespace Render {

PipelineState::PipelineState(void)
: mPipelineState(nullptr)
{

}


PipelineState::~PipelineState( void) {
    ReleaseAndSetNull(mPipelineState);
}

GraphicsState::GraphicsState(D3D12_PIPELINE_STATE_FLAGS flag)
: PipelineState()
{
    Initialize(flag);
}

GraphicsState::~GraphicsState(void) {

}

void GraphicsState::Initialize(D3D12_PIPELINE_STATE_FLAGS flag) {
    mDesc = {};
    mDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    mDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    mDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    mDesc.SampleMask = UINT_MAX;
    mDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    mDesc.NumRenderTargets = 1;
    mDesc.RTVFormats[0] = gRenderTargetFormat;
    mDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    FillSampleDesc(mDesc.SampleDesc, gRenderTargetFormat);
    mDesc.NodeMask = 1;
    mDesc.Flags = flag;

    mDesc.RasterizerState.MultisampleEnable = (gMSAAState != DisableMSAA);
}

void GraphicsState::Create(RootSignature *rootSignature) {
    ASSERT_PRINT((rootSignature != nullptr && rootSignature->Get() != nullptr));
    ReleaseAndSetNull(mPipelineState);

    mDesc.pRootSignature = rootSignature->Get();
    ASSERT_SUCCEEDED(gDevice->CreateGraphicsPipelineState(&mDesc, IID_PPV_ARGS(&mPipelineState)));
}

ComputeState::ComputeState(D3D12_PIPELINE_STATE_FLAGS flag)
: PipelineState()
{
    Initialize(flag);
}

ComputeState::~ComputeState(void) {

}

void ComputeState::Initialize(D3D12_PIPELINE_STATE_FLAGS flag) {
    mDesc = {};
    mDesc.NodeMask = 1;
    mDesc.Flags = flag;
}

void ComputeState::Create(RootSignature *rootSignature) {
    ASSERT_PRINT((rootSignature != nullptr && rootSignature->Get() != nullptr));
    ReleaseAndSetNull(mPipelineState);

    mDesc.pRootSignature = rootSignature->Get();
    ASSERT_SUCCEEDED(gDevice->CreateComputePipelineState(&mDesc, IID_PPV_ARGS(&mPipelineState)));
}

}

