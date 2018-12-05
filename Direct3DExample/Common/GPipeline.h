#pragma once

class GPipeline {
public:
    enum RootParamType {
        kRootParamConstBuffer,
        kRootParamTexture,
        kRootParamSampler,
        kRootParamTypeMax
    };

    GPipeline(void);
    ~GPipeline(void);

    inline ID3D12RootSignature * GetRS(void) { return mRootSignature; }
    inline ID3D12PipelineState * GetPS(void) { return mPipelineState; }

    void AddRootParam(RootParamType type, uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void AddInputElement(D3D12_INPUT_ELEMENT_DESC &element);

    void Generate(void);

private:
    void InitDescribtors(void);

    void AddConstBufferParam(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility);
    void AddTextureParam(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility);
    void AddSamplerParam(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility);

    // describtor
    CList<D3D12_DESCRIPTOR_RANGE1> mRSRanges;
    CList<D3D12_ROOT_PARAMETER1>mRSParams;
    CHashString mVSName;
    CHashString mPSName;
    CHashString mDSName;
    CHashString mHSName;
    CHashString mGSName;
    CList<D3D12_INPUT_ELEMENT_DESC> mInputDescs;
    D3D12_RASTERIZER_DESC mRasterizerDesc;
    D3D12_BLEND_DESC mBlendDesc;
    D3D12_DEPTH_STENCIL_DESC mDepthStencilDesc;
    DXGI_SAMPLE_DESC mSampleDesc;

    ID3D12RootSignature *mRootSignature;
    ID3D12PipelineState *mPipelineState;

    CHashString m1;
};

