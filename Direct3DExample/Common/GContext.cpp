#include "pch.h"
#include "GUploader.h"
#include "GContext.h"

GContext *GContext::msContext = nullptr;

void GContext::CreateDefault(HWND hwnd) {
    assert(msContext == nullptr);
    msContext = new GContext(hwnd);
}

GContext::GContext(HWND hwnd)
:mHwnd(hwnd)
{
    WINDOWINFO windowInfo;
    GetWindowInfo(mHwnd, &windowInfo);
    mWidth = windowInfo.rcClient.right - windowInfo.rcClient.left;
    mHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;
    mViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    mScissorRect = { 0, 0, static_cast<long>(mWidth), static_cast<long>(mHeight) };

    Initialize();
}

GContext::~GContext(void) {
    delete mUploader;

    CloseHandle(mFenceEvent);
    ReleaseIfNotNull(mFence);

    ReleaseIfNotNull(mBundleAllocator);
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        ReleaseIfNotNull(mCommandAllocators[i]);
    }

    ReleaseIfNotNull(mDepthStencil);
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        ReleaseIfNotNull(mRenderTargets[i]);
    }
    ReleaseIfNotNull(mDsvHeap);
    ReleaseIfNotNull(mRtvHeap);

    ReleaseIfNotNull(mSwapChain);
    ReleaseIfNotNull(mCommandQueue);
    ReleaseIfNotNull(mDevice);

#if defined(_DEBUG)
    IDXGIDebug1 *debug = nullptr;
    HRESULT result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
    if (SUCCEEDED(result)) {
        debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        ReleaseIfNotNull(debug);
    }
#endif
}

void GContext::Initialize(void) {
    // debug layer
    uint32_t dxgiFlag = 0;
#if defined(_DEBUG)
    ID3D12Debug *debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        dxgiFlag |= DXGI_CREATE_FACTORY_DEBUG;
        debug->EnableDebugLayer();
        debug->Release();
    }
#endif
    // dxgi
    IDXGIFactory4 *factory = nullptr;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFlag, IID_PPV_ARGS(&factory)));

    // device
    IDXGIAdapter1 *adapter = nullptr;
    ThrowIfFailed(GetHardwareAdapter(factory, &adapter));
    ThrowIfFailed(D3D12CreateDevice(adapter, FEATURE_LEVEL, IID_PPV_ARGS(&mDevice)));
    adapter->Release();

    // command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    // swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = mWidth;
    swapChainDesc.Height = mHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    IDXGISwapChain1 *swapChain = nullptr;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(mCommandQueue, mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain));

    // bind window
    ThrowIfFailed(factory->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER));
    factory->Release();

    swapChain->QueryInterface(IID_PPV_ARGS(&mSwapChain));
    swapChain->Release();

    mCurFrame = mSwapChain->GetCurrentBackBufferIndex();

    // render target view heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FRAME_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));
    mRtvDescSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // render target views
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&(mRenderTargets[i]))));
        mDevice->CreateRenderTargetView(mRenderTargets[i], nullptr, rtvHandle);
        rtvHandle.ptr += mRtvDescSize;
    }

    // depth stencil view heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));

    // depth stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DEPTH_STENCIL_FORMAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
    D3D12_CLEAR_VALUE depthStencilClearValue = {};
    depthStencilClearValue.Format = DEPTH_STENCIL_FORMAT;
    depthStencilClearValue.DepthStencil.Depth = 1.0f;
    depthStencilClearValue.DepthStencil.Stencil = 0;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DEPTH_STENCIL_FORMAT, mWidth, mHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ThrowIfFailed(mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthStencilClearValue, IID_PPV_ARGS(&mDepthStencil)));
    mDevice->CreateDepthStencilView(mDepthStencil, &depthStencilDesc, mDsvHeap->GetCPUDescriptorHandleForHeapStart());

    // command allocator
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(mCommandAllocators[i]))));
    }
    ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mBundleAllocator)));

    // fence
    ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        mFenceValues[i] = 1;
    }

    // uploader
    mUploader = new GUploader(mDevice);
}

HRESULT GContext::GetHardwareAdapter(IDXGIFactory2 *factory, IDXGIAdapter1 **pAdapter) {
    IDXGIAdapter1 *adapter = nullptr;
    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter, FEATURE_LEVEL, __uuidof(ID3D12Device), nullptr))) {
            break;
        }

        adapter->Release();
    }

    *pAdapter = adapter;

    return adapter != nullptr ? S_OK : -1;
}

void GContext::MoveToNextFrame(void) {
    const uint64_t fenceValue = mFenceValues[mCurFrame];
    mCommandQueue->Signal(mFence, fenceValue);

    mCurFrame = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFenceValues[mCurFrame]) {
        mFence->SetEventOnCompletion(mFenceValues[mCurFrame], mFenceEvent);
        WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
    }

    mFenceValues[mCurFrame] = fenceValue + 1;
}

