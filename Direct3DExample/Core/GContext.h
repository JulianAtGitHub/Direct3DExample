#pragma once

class GUploader;

class GContext {
public:
    static void CreateDefault(HWND hwnd);
    static inline GContext * Default(void) { return msContext; }

    const static D3D_FEATURE_LEVEL FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_0;
    const static uint32_t FRAME_COUNT = 2;
    const static DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT;

    inline GUploader * GetUploader(void) { return mUploader; }
    inline ID3D12Device * GetDevice(void) { return mDevice; }

private:
    GContext(HWND hwnd);
    ~GContext(void);

    void Initialize(void);

    HRESULT GetHardwareAdapter(IDXGIFactory2 *factory, IDXGIAdapter1 **pAdapter);

    void MoveToNextFrame(void);

    static GContext *msContext;

    HWND mHwnd;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mCurFrame;
    uint32_t mRtvDescSize;
    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
    // device
    ID3D12Device *mDevice;
    ID3D12CommandQueue *mCommandQueue;
    IDXGISwapChain3 *mSwapChain;
    // render target
    ID3D12DescriptorHeap *mRtvHeap;
    ID3D12Resource *mRenderTargets[FRAME_COUNT];
    // depth stencil
    ID3D12DescriptorHeap *mDsvHeap;
    ID3D12Resource *mDepthStencil;
    // commands alloctor
    ID3D12CommandAllocator *mCommandAllocators[FRAME_COUNT];
    ID3D12CommandAllocator *mBundleAllocator;
    // synchronisation
    ID3D12Fence *mFence;
    HANDLE mFenceEvent;
    uint64_t mFenceValues[FRAME_COUNT];

    GUploader *mUploader;
};
