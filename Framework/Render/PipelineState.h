#pragma once

namespace Render {

class RootSignature;

class PipelineState {
public:
    PipelineState(void);
    virtual ~PipelineState( void);

    INLINE ID3D12PipelineState * GetPipelineState(void) const { return mPipelineState; }
    virtual void Create(RootSignature *rootSignature) = 0;

protected:
    INLINE void SetShaderByteCode(D3D12_SHADER_BYTECODE &shader, const void *code, size_t length) { shader.pShaderBytecode = code; shader.BytecodeLength = length; }

    ID3D12PipelineState        *mPipelineState;
};

class GraphicsState : public PipelineState {
public:
    GraphicsState(D3D12_PIPELINE_STATE_FLAGS flag = D3D12_PIPELINE_STATE_FLAG_NONE);
    virtual ~GraphicsState(void);

    INLINE void EnableDepth(bool enable) { mDesc.DepthStencilState.DepthEnable = enable; }
    INLINE void EnableStencil(bool enable) { mDesc.DepthStencilState.StencilEnable = enable; }

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

    INLINE void SetVertexShader(const void *byteCode, size_t length) { SetShaderByteCode(mDesc.VS, byteCode, length); }
    INLINE void SetPixelShader(const void *byteCode, size_t length) { SetShaderByteCode(mDesc.PS, byteCode, length); }
    INLINE void SetGeometryShader(const void *byteCode, size_t length) { SetShaderByteCode(mDesc.GS, byteCode, length); }
    INLINE void SetHullShader(const void *byteCode, size_t length) { SetShaderByteCode(mDesc.HS, byteCode, length); }
    INLINE void SetDomainShader(const void *byteCode, size_t length) { SetShaderByteCode(mDesc.DS, byteCode, length); }

    INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveType(void) const { return mDesc.PrimitiveTopologyType; };
    INLINE void SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type) { mDesc.PrimitiveTopologyType = type; };
    INLINE DXGI_FORMAT GetDepthStencilFormat(void) const { return mDesc.DSVFormat; }
    INLINE void SetDepthStencilFormat(DXGI_FORMAT format) { mDesc.DSVFormat = format; }

    void Create(RootSignature *rootSignature);

private:
    void Initialize(D3D12_PIPELINE_STATE_FLAGS flag);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC  mDesc;
};

class ComputeState : public PipelineState {
public:
    ComputeState(D3D12_PIPELINE_STATE_FLAGS flag = D3D12_PIPELINE_STATE_FLAG_NONE);
    virtual ~ComputeState(void);

    INLINE D3D12_CACHED_PIPELINE_STATE & GetCachedPSO(void) { return mDesc.CachedPSO; }

    INLINE D3D12_SHADER_BYTECODE & GetShader(void) { return mDesc.CS; }

    INLINE void SetShader(const void *byteCode, size_t length) { SetShaderByteCode(mDesc.CS, byteCode, length); }

    void Create(RootSignature *rootSignature);

private:
    void Initialize(D3D12_PIPELINE_STATE_FLAGS flag);

    D3D12_COMPUTE_PIPELINE_STATE_DESC   mDesc;
};

}