#pragma once

namespace Render {

enum MSAAState {
    DisableMSAA,
    Enable2xMSAA,
    Enable4xMSAA,
    Enable8xMSAA,
    Enable16xMSAA,
    MSAAStateMax
};

struct DescriptorHandle {
    DescriptorHandle(SIZE_T cpuPtr = 0, UINT64 gpuPtr = 0) {
        cpu.ptr = cpuPtr;
        gpu.ptr = gpuPtr;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

class CommandContext;
class DescriptorHeap;
class RenderTargetBuffer;
class DepthStencilBuffer;

constexpr uint32_t          FRAME_COUNT = 3;

extern ID3D12Device        *gDevice;
extern ID3D12Device5       *gDXRDevice;
extern IDXGISwapChain3     *gSwapChain;
extern CommandContext      *gCommand;

extern RenderTargetBuffer  *gRenderTarget[FRAME_COUNT];
extern RenderTargetBuffer  *gRenderTargetMSAA[FRAME_COUNT];
extern DepthStencilBuffer  *gDepthStencil;

extern DXGI_FORMAT          gRenderTargetFormat;
extern MSAAState            gMSAAState;

// feature abilities
extern bool                 gRootSignatureSupport_Version_1_1;
extern bool                 gTypedUAVLoadSupport_R11G11B10_FLOAT;
extern bool                 gTypedUAVLoadSupport_R16G16_FLOAT;
extern bool                 gTypedUAVLoadSupport_R16G16B16A16_FLOAT;
extern bool                 gMSAASupport[MSAAStateMax];
extern bool                 gHDROutputSupport;
extern bool                 gRayTracingSupport;

extern void Initialize(HWND hwnd, MSAAState msaa = DisableMSAA);
extern void Terminate(void);

extern void FillSampleDesc(DXGI_SAMPLE_DESC &desc, DXGI_FORMAT format);
extern uint32_t BitsPerPixel(DXGI_FORMAT format);
extern uint32_t BytesPerPixel(DXGI_FORMAT format);
extern bool IsSRGB(DXGI_FORMAT format);

}