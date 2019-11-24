#pragma once

class SkyboxPass {
public:
    SkyboxPass(void);
    ~SkyboxPass(void);

    void Update(uint32_t currentFrame, const XMFLOAT4X4 &viewMatrix, const XMFLOAT4X4 projMatrix);
    void Render(uint32_t currentFrame, Render::DescriptorHeap *envTexHeap, uint32_t envTexIndex);

private:
    void Initialize(void);
    void Destroy(void);

    struct TransformCB {
        XMFLOAT4X4 ViewMat;
        XMFLOAT4X4 ProjMat;
    };

    static constexpr uint32_t VERTEX_COUNT = 24;
    static constexpr uint32_t INDEX_COUNT = 36;

    static XMFLOAT3 msVertices[VERTEX_COUNT];
    static uint16_t msIndices[INDEX_COUNT];

    Render::RootSignature      *mRootSignature;
    Render::GraphicsState      *mGraphicsState;
    Render::ConstantBuffer     *mConstBuffer;
    Render::DescriptorHeap     *mSamplerHeap;
    Render::Sampler            *mSampler;
    Render::GPUBuffer          *mVertexBuffer;
    Render::GPUBuffer          *mIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW    mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW     mIndexBufferView;
};
