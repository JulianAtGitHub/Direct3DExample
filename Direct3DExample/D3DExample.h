#pragma once

#include "Example.h"

class D3DExample : public Example {
public:
    D3DExample(HWND hwnd);
    virtual ~D3DExample(void);

    virtual void Init(void);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

private:
    void LoadPipeline(void);
    void LoadAssets(void);
    void PopulateCommandList(void);
    void MoveToNextFrame(void);

    Render::RootSignature *mRootSignature;
    Render::GraphicsState *mGraphicsState;

    Render::GPUBuffer *mVertexIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    Render::ConstantBuffer *mConstBuffer;

    Render::DescriptorHeap *mShaderResourceHeap;
    std::vector<Render::PixelBuffer *> mTextures;

    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler *mSampler;

    uint64_t mFenceValues[Render::FRAME_COUNT];
    uint32_t mCurrentFrame;

    uint32_t mWidth;
    uint32_t mHeight;

    Utils::Scene *mScene;
};
