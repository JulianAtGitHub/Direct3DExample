#include "stdafx.h"
#include "RenderCore.h"
#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorHeap.h"
#include "Resource/RenderTargetBuffer.h"
#include "Resource/DepthStencilBuffer.h"

namespace Render {

ID3D12Device       *gDevice                     = nullptr;
ID3D12Device5      *gDXRDevice                  = nullptr;
IDXGISwapChain3    *gSwapChain                  = nullptr;
CommandContext     *gCommand                    = nullptr;

DescriptorHeap     *gRenderTargetHeap           = nullptr;
DescriptorHeap     *gDepthStencilHeap           = nullptr;

RenderTargetBuffer *gRenderTarget[FRAME_COUNT]  = { nullptr };
DepthStencilBuffer *gDepthStencil               = nullptr;

// feature abilities
bool                gRootSignatureSupport_Version_1_1 = false;
bool                gTypedUAVLoadSupport_R11G11B10_FLOAT = false;
bool                gTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;
bool                gHDROutputSupport = false;
bool                gRayTracingSupport = false;

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
    SIZE_T maxMemSize = 0;
    D3D_FEATURE_LEVEL deviceLevels[] = {D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_0 };

    WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;
    WRL::ComPtr<ID3D12Device> d3dDevice;
    ASSERT_SUCCEEDED(CreateDXGIFactory2(dxgiFlag, IID_PPV_ARGS(&dxgiFactory)));
    for (uint32_t i = 0; i < _countof(deviceLevels); ++i) {
        for (uint32_t j = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(j, &dxgiAdapter); ++j) {
            DXGI_ADAPTER_DESC1 desc;
            dxgiAdapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (desc.DedicatedVideoMemory > maxMemSize && SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), deviceLevels[i], IID_PPV_ARGS(&d3dDevice)))) {
                char *description = WStr2Str(desc.Description);
                Print("D3D12-capable hardware found: %s (%u MB)\n", description, desc.DedicatedVideoMemory >> 20);
                free(description);
                maxMemSize = desc.DedicatedVideoMemory;
            }
        }

        if (maxMemSize > 0) {
            Print("Device feature level: 0x%x\n", deviceLevels[i]);
            gDevice = d3dDevice.Detach();
            break;
        }
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

    {
        // Root signature 1.1 support check
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
            gRootSignatureSupport_Version_1_1 = true;
        }
    }

    {
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
    }

    {
        // check if support ray tracing
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData = {};
        if(SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5)))
            && featureData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
            gRayTracingSupport = true;
            ASSERT_SUCCEEDED(gDevice->QueryInterface(IID_PPV_ARGS(&gDXRDevice)));
        }
    }

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
    ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(gCommand->GetQueue()->Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
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
    gRenderTargetHeap = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_COUNT);
    gDepthStencilHeap = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        ID3D12Resource *resource = nullptr;
        ASSERT_SUCCEEDED(gSwapChain->GetBuffer(i, IID_PPV_ARGS(&resource)));
        gRenderTarget[i] = new RenderTargetBuffer(resource);
        gRenderTarget[i]->CreateView(gRenderTargetHeap->Allocate());
    }
    gDepthStencil = new DepthStencilBuffer(width, height, DXGI_FORMAT_D32_FLOAT);
    gDepthStencil->CreateView(gDepthStencilHeap->Allocate());
}

void Terminate(void) {
    DeleteAndSetNull(gDepthStencilHeap);
    DeleteAndSetNull(gRenderTargetHeap);

    DeleteAndSetNull(gDepthStencil);
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        DeleteAndSetNull(gRenderTarget[i]);
    }

    DeleteAndSetNull(gCommand);
    ReleaseAndSetNull(gSwapChain);
    ReleaseAndSetNull(gDevice);
    ReleaseAndSetNull(gDXRDevice);
}

uint32_t BytesPerPixel(DXGI_FORMAT format) {
    return (BitsPerPixel(format) >> 3);
}

uint32_t BitsPerPixel(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}

bool IsSRGB(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;
        default:
            return false;
    }
}

}