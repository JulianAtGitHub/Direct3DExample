#pragma once

#include "Example.h"
#include "Core/Render/RenderCore.h"

namespace Model {
    class Scene;
}

namespace Render {
    class GPUBuffer;
    class ConstantBuffer;
    class PixelBuffer;
    class DescriptorHeap;
    class Sampler;
    class GraphicsState;
    class RootSignature;
}

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
    ID3D12CommandAllocator *mBundleAllocator;
    ID3D12GraphicsCommandList *mBundles[Render::FRAME_COUNT];

    Render::GPUBuffer *mVertexIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    Render::ConstantBuffer *mConstBuffer;

    Render::DescriptorHeap *mShaderResourceHeap;
    CList<Render::PixelBuffer *> mTextures;

    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler *mSampler;

    uint64_t mFenceValues[Render::FRAME_COUNT];
    uint32_t mCurrentFrame;

    uint32_t mWidth;
    uint32_t mHeight;

    Model::Scene *mScene;
};