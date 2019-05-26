#pragma once

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
}

class AnExample {
public:
    AnExample(HWND hwnd);
    ~AnExample(void);

    void Init(void);
    void Update(void);
    void Render(void);
    void Destroy(void);

    void OnKeyDown(uint8_t /*key*/)   {}
    void OnKeyUp(uint8_t /*key*/)     {}

private:
    void PopulateCommandList(void);
    uint8_t * GenerateTextureData(void);

    void MoveToNextFrame(void);

    void LoadPipeline(void);
    void LoadAssets(void);

    static const uint32_t TEXTURE_WIDTH = 256;
    static const uint32_t TEXTURE_HEIGHT = 256;
    static const uint32_t TEXTURE_PIXEL_SIZE = 4;

    HWND mHwnd;

    ID3D12RootSignature *mRootSignature;
    ID3D12PipelineState *mPipelineState;
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
