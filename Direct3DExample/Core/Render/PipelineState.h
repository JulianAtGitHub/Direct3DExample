#pragma once

namespace Render {

class PipelineState {
public:
    PipelineState(void);
    virtual ~PipelineState( void);

    INLINE const ID3D12RootSignature * GetRootSignature(void) const { return mRootSignature; }
    INLINE void SetRootSignature(const ID3D12RootSignature *rootSignature) { mRootSignature = rootSignature; }
    INLINE ID3D12PipelineState * GetPipelineState(void) const { return mPipelineState; }

protected:
    const ID3D12RootSignature  *mRootSignature;
    ID3D12PipelineState        *mPipelineState;
};

class GraphicsState : public PipelineState {
public:
    GraphicsState(void);
    virtual ~GraphicsState(void);

    INLINE D3D12_SHADER_BYTECODE & GetVertexShader(void) { return mDesc.VS; }
    INLINE D3D12_SHADER_BYTECODE & GetPixelShader(void) { return mDesc.PS; }
    INLINE D3D12_SHADER_BYTECODE & GetGeometryShader(void) { return mDesc.GS; }
    INLINE D3D12_SHADER_BYTECODE & GetHullShader(void) { return mDesc.HS; }
    INLINE D3D12_SHADER_BYTECODE & GetDomainShader(void) { return mDesc.DS; }

    INLINE D3D12_STREAM_OUTPUT_DESC & GetStreamOutput(void) { return mDesc.StreamOutput; }
    INLINE D3D12_BLEND_DESC & GetBlendState(void) { return mDesc.BlendState; }
    INLINE D3D12_RASTERIZER_DESC & GetRasterizerState(void) { return mDesc.RasterizerState; }
    INLINE D3D12_DEPTH_STENCIL_DESC & GetDepthStencilState(void) { return mDesc.DepthStencilState; }
    INLINE D3D12_INPUT_LAYOUT_DESC & GetInputLayout(void) { return mDesc.InputLayout; }
    INLINE DXGI_SAMPLE_DESC & GetSampleDesc(void) { return mDesc.SampleDesc; }
    INLINE D3D12_CACHED_PIPELINE_STATE & GetCachedPSO(void) { return mDesc.CachedPSO; }

    INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveType(void) const { return mDesc.PrimitiveTopologyType; };
    INLINE void SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type) { mDesc.PrimitiveTopologyType = type; };
    INLINE DXGI_FORMAT GetDepthStencilFormat(void) const { return mDesc.DSVFormat; }
    INLINE void SetDepthStencilFormat(DXGI_FORMAT format) { mDesc.DSVFormat = format; }

    void Create(void);

private:
    void Initialize(void);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC  mDesc;
};

}