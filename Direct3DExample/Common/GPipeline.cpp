#include "pch.h"
#include "GContext.h"
#include "GShaderLib.h"
#include "GPipeline.h"

GPipeline::GPipeline(void) 
:mRootSignature(nullptr)
,mPipelineState(nullptr)
{
    InitDescribtors();
}

GPipeline::~GPipeline(void) {
    ReleaseIfNotNull(mPipelineState);
    ReleaseIfNotNull(mRootSignature);
}

void GPipeline::AddRootParam(RootParamType type, uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    switch (type)
    {
    case GPipeline::kRootParamConstBuffer:
        AddConstBufferParam(shaderRegister, visibility);
        break;
    case GPipeline::kRootParamTexture:
        AddTextureParam(shaderRegister, visibility);
        break;
    case GPipeline::kRootParamSampler:
        AddSamplerParam(shaderRegister, visibility);
        break;
    default:
        LogOutput("Invalid RootParamType: %d\n", type);
        break;
    }
}

void GPipeline::AddInputElement(D3D12_INPUT_ELEMENT_DESC &element) {
    mInputDescs.PushBack(element);
}

void GPipeline::InitDescribtors(void) {
    mRasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    mBlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    mDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    mSampleDesc = {};
}

void GPipeline::AddConstBufferParam(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    CD3DX12_ROOT_PARAMETER1 param = {};
    param.InitAsConstantBufferView(shaderRegister, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, visibility);
    mRSParams.PushBack(param);
}

void GPipeline::AddTextureParam(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    CD3DX12_DESCRIPTOR_RANGE1 range;
    range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, shaderRegister, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    mRSRanges.PushBack(range);

    CD3DX12_ROOT_PARAMETER1 param = {};
    param.InitAsDescriptorTable(1, &(mRSRanges.Last()), visibility);
    mRSParams.PushBack(param);
}

void GPipeline::AddSamplerParam(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    CD3DX12_DESCRIPTOR_RANGE1 range;
    range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, shaderRegister);
    mRSRanges.PushBack(range);

    CD3DX12_ROOT_PARAMETER1 param = {};
    param.InitAsDescriptorTable(1, &(mRSRanges.Last()), visibility);
    mRSParams.PushBack(param);
}

void GPipeline::Generate(void) {

    ID3D12Device *device = GContext::Default()->GetDevice();
    GShaderLib *shaderLib = GShaderLib::Default();

    // root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(mRSParams.Count(), mRSParams.Data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob *signature = nullptr;
    ID3DBlob *error = nullptr;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

    ReleaseIfNotNull(signature);
    ReleaseIfNotNull(error);

    // pipeline state
    D3D12_INPUT_LAYOUT_DESC inputDesc = {};
    inputDesc.pInputElementDescs = mInputDescs.Data();
    inputDesc.NumElements = mInputDescs.Count();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};

    ID3DBlob *vertexShader = shaderLib->FindShader(mVSName);
    if (vertexShader) {
        pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
    }

    ID3DBlob *pixelShader = shaderLib->FindShader(mPSName);
    if (pixelShader) {
        pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
    }

    ID3DBlob *domainShader = shaderLib->FindShader(mDSName);
    if (domainShader) {
        pipelineStateDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader);
    }

    ID3DBlob *hullShader = shaderLib->FindShader(mHSName);
    if (hullShader) {
        pipelineStateDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader);
    }

    ID3DBlob *geometryShader = shaderLib->FindShader(mGSName);
    if (geometryShader) {
        pipelineStateDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShader);
    }

    pipelineStateDesc.InputLayout = inputDesc;
    pipelineStateDesc.pRootSignature = mRootSignature;
    pipelineStateDesc.RasterizerState = mRasterizerDesc;
    pipelineStateDesc.BlendState = mBlendDesc;
    pipelineStateDesc.DepthStencilState = mDepthStencilDesc;
    pipelineStateDesc.SampleMask = UINT_MAX;
    pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateDesc.NumRenderTargets = 1;
    pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineStateDesc.DSVFormat = GContext::DEPTH_STENCIL_FORMAT;
    pipelineStateDesc.SampleDesc = mSampleDesc;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&mPipelineState)));
}