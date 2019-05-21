#include "pch.h"
#include "RenderCore.h"
#include "CommandContext.h"
#include "DescriptorHeap.h"
#include "RenderTargetBuffer.h"
#include "DepthStencilBuffer.h"

namespace Render {

ID3D12Device       *gDevice                     = nullptr;
IDXGISwapChain3    *gSwapChain                  = nullptr;
ID3D12CommandQueue *gCommandQueue               = nullptr;
CommandContext     *gCommand                    = nullptr;

DescriptorHeap     *gRenderTargetHeap           = nullptr;
DescriptorHeap     *gDepthStencilHeap           = nullptr;

RenderTargetBuffer *gRenderTarget[FRAME_COUNT]  = { nullptr };
DepthStencilBuffer *gDepthStencil               = nullptr;

// feature abilities
bool                gTypedUAVLoadSupport_R11G11B10_FLOAT = false;
bool                gTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;
bool                gHDROutputSupport = false;

void Initialize(HWND hwnd) {
    // fill view port
    WINDOWINFO windowInfo;
    GetWindowInfo(hwnd, &windowInfo);
    LONG width = windowInfo.rcClient.right - windowInfo.rcClient.left;
    LONG height = windowInfo.rcClient.bottom - windowInfo.rcClient.top;

    // enable debug layer
#if _DEBUG
    WRL::ComPtr<ID3D12Debug> debugInterface;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
        debugInterface->EnableDebugLayer();
    } else {
       Print("Unable to enable D3D12 debug validation layer\n");
    }
#endif

    // create device
    UINT dxgiFlag = 0;
    WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    ASSERT_SUCCEEDED(CreateDXGIFactory2(dxgiFlag, IID_PPV_ARGS(&dxgiFactory)));

    WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;
    WRL::ComPtr<ID3D12Device> d3dDevice;
    SIZE_T maxMemSize = 0;
    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(i, &dxgiAdapter); ++i) {
        DXGI_ADAPTER_DESC1 desc;
        dxgiAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        if (desc.DedicatedVideoMemory > maxMemSize && SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice)))) {
            dxgiAdapter->GetDesc1(&desc);
            char *description = WStr2Str(desc.Description);
            Print("D3D12-capable hardware found:  %s (%u MB)\n", description, desc.DedicatedVideoMemory >> 20);
            free(description);
            maxMemSize = desc.DedicatedVideoMemory;
        }
    }

    if (maxMemSize > 0) {
        gDevice = d3dDevice.Detach();
    }

    // using software instead
    if (!gDevice) {
        Print("WARP software adapter requested.  Initializing...\n");
        ASSERT_SUCCEEDED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter)));
        ASSERT_SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice)));
        gDevice = d3dDevice.Detach();
    }

#if _DEBUG
    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(gDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY severities[] =  {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID denyIds[] = {
            // This occurs when there are uninitialized descriptors in a descriptor table, even when a
            // shader does not access the missing descriptors.  I find this is common when switching
            // shader permutations and not wanting to change much code to reorder resources.
            D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

            // Triggered when a shader does not export all color components of a render target, such as
            // when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
            D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

            // This occurs when a descriptor table is unbound even when a shader does not access the missing
            // descriptors.  This is common with a root signature shared between disparate shaders that
            // don't all need the same types of resources.
            D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

            // (D3D12_MESSAGE_ID)1008,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
        };

        D3D12_INFO_QUEUE_FILTER newFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        newFilter.DenyList.NumSeverities = _countof(severities);
        newFilter.DenyList.pSeverityList = severities;
        newFilter.DenyList.NumIDs = _countof(denyIds);
        newFilter.DenyList.pIDList = denyIds;

        infoQueue->PushStorageFilter(&newFilter);
        infoQueue->Release();
    }
#endif

    // We like to do read-modify-write operations on UAVs during post processing.  To support that, we
    // need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
    // decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
    // load support.
    D3D12_FEATURE_DATA_D3D12_OPTIONS featureData = {};
    if (SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureData, sizeof(featureData)))) {
        if (featureData.TypedUAVLoadAdditionalFormats) {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT support = {
                DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
            };

            if (SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support))) &&
                (support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0) {
                gTypedUAVLoadSupport_R11G11B10_FLOAT = true;
            }

            support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

            if (SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support))) &&
                (support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0) {
                gTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
            }
        }
    }

    // command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ASSERT_SUCCEEDED(gDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&gCommandQueue)));

    gCommand = new CommandContext(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    WRL::ComPtr<IDXGISwapChain1> swapChain1;
    ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(gCommandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    ASSERT_SUCCEEDED(swapChain1->QueryInterface(IID_PPV_ARGS(&gSwapChain)));

#if defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
    {
        IDXGISwapChain4* swapChain4 = (IDXGISwapChain4*)swapChain1.Get();
        WRL::ComPtr<IDXGIOutput> output;
        WRL::ComPtr<IDXGIOutput6> output6;
        DXGI_OUTPUT_DESC1 outputDesc;
        UINT colorSpaceSupport;

        // Query support for ST.2084 on the display and set the color space accordingly
        if (SUCCEEDED(swapChain4->GetContainingOutput(&output))
            && SUCCEEDED(output.As(&output6))
            && SUCCEEDED(output6->GetDesc1(&outputDesc))
            && outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020
            && SUCCEEDED(swapChain4->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport))
            && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
            && SUCCEEDED(swapChain4->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020))) {
            gHDROutputSupport = true;
        }
    }
#endif

    // frame buffer
    gRenderTargetHeap = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 16);
    gDepthStencilHeap = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 8);

    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        ID3D12Resource *resource = nullptr;
        ASSERT_SUCCEEDED(gSwapChain->GetBuffer(i, IID_PPV_ARGS(&resource)));
        gRenderTarget[i] = new RenderTargetBuffer();
        gRenderTarget[i]->CreateFromSwapChain(resource, gRenderTargetHeap->Allocate());
    }
    gDepthStencil = new DepthStencilBuffer();
    gDepthStencil->Create(width, height, DXGI_FORMAT_D32_FLOAT, gDepthStencilHeap->Allocate());
}

void Terminate(void) {
    DeleteAndSetNull(gDepthStencilHeap);
    DeleteAndSetNull(gRenderTargetHeap);

    DeleteAndSetNull(gDepthStencil);
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        DeleteAndSetNull(gRenderTarget[i]);
    }

    DeleteAndSetNull(gCommand);
    ReleaseAndSetNull(gCommandQueue);
    ReleaseAndSetNull(gSwapChain);
    ReleaseAndSetNull(gDevice);
}

ID3D12Resource * GetRenderTarget(uint32_t frameIndex) { 
    ASSERT_PRINT(frameIndex < FRAME_COUNT);
    return gRenderTarget[frameIndex]->GetResource();
}

const D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle(uint32_t frameIndex) { 
    ASSERT_PRINT(frameIndex < FRAME_COUNT);
    return gRenderTarget[frameIndex]->GetHandle().cpu;
}

const D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilViewHandle(void) {
    return gDepthStencil->GetHandle().cpu;
}

}