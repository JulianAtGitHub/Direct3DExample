#pragma once

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
    void GetHardwareAdapter(IDXGIFactory2 *factory, IDXGIAdapter1 **pAdapter);
    void PopulateCommandList(void);
    uint8_t * GenerateTextureData(void);

    void WaitForGPU(void);
    void MoveToNextFrame(void);

    void LoadPipeline(void);
    void LoadAssets(void);

    static const D3D_FEATURE_LEVEL FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_0;

    static const uint32_t FRAME_COUNT = 3;

    static const uint32_t TEXTURE_WIDTH = 256;
    static const uint32_t TEXTURE_HEIGHT = 256;
    static const uint32_t TEXTURE_PIXEL_SIZE = 4;

    HWND mHwnd;
    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
    IDXGISwapChain3 *mSwapChain;
    ID3D12Device *mDevice;
    ID3D12Resource *mRenderTargets[FRAME_COUNT];
    ID3D12Resource *mDepthStencil;
    ID3D12CommandAllocator *mCommandAllocators[FRAME_COUNT];
    ID3D12CommandAllocator *mBundleAllocator;
    ID3D12CommandQueue *mCommandQueue;
    ID3D12DescriptorHeap *mRtvHeap; // render target view heap
    ID3D12DescriptorHeap *mDsvHeap; // depth stencil view heap
    ID3D12DescriptorHeap *mCbvSrvHeap; // const buffer view and shader resource view heap
    ID3D12DescriptorHeap *mSamplerHeap;

    ID3D12RootSignature *mRootSignature;
    ID3D12PipelineState *mPipelineState;
    ID3D12GraphicsCommandList *mCommandList;
    ID3D12GraphicsCommandList *mBundles[FRAME_COUNT];

    ID3D12Resource *mVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    ID3D12Resource *mIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    ID3D12Resource *mConstBuffer;
    ID3D12Resource *mTexture;

    HANDLE mFenceEvent;
    ID3D12Fence *mFence;
    uint64_t mFenceValues[FRAME_COUNT];

    uint32_t mCurrentFrame;
    uint32_t mRtvDescSize;
    uint32_t mConstBufferSize;
    void *mDataBeginCB[FRAME_COUNT];

    uint32_t mWidth;
    uint32_t mHeight;
    float mAspectRatio;
};
