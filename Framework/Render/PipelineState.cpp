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

GraphicsState::GraphicsState(void)
: PipelineState()
{
    Initialize();
}

GraphicsState::~GraphicsState(void) {

}

void GraphicsState::Initialize(void) {

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;

    mDesc = {};
    mDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    mDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    mDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    mDesc.SampleMask = UINT_MAX;
    mDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    mDesc.NumRenderTargets = 1;
    mDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    mDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    mDesc.SampleDesc = sampleDesc;
    mDesc.NodeMask = 1;
}

void GraphicsState::LoadShaderByteCode(const char *fileName, D3D12_SHADER_BYTECODE &byteCode) {
    if (byteCode.pShaderBytecode) {
        free(const_cast<void *>(byteCode.pShaderBytecode));
    }
    byteCode.pShaderBytecode = ReadFileData(fileName, byteCode.BytecodeLength);
}

void GraphicsState::Create(RootSignature *rootSignature) {
    ASSERT_PRINT((rootSignature != nullptr && rootSignature->Get() != nullptr));
    ReleaseAndSetNull(mPipelineState);

    mDesc.pRootSignature = rootSignature->Get();
    ASSERT_SUCCEEDED(gDevice->CreateGraphicsPipelineState(&mDesc, IID_PPV_ARGS(&mPipelineState)));
}

}

