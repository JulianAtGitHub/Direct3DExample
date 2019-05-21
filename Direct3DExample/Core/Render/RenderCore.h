#pragma once

#include "DescriptorHeap.h"
#include "RenderTargetBuffer.h"
#include "DepthStencilBuffer.h"

namespace Render {

class CommandContext;
class DescriptorHeap;
class RenderTargetBuffer;
class DepthStencilBuffer;

constexpr uint32_t          FRAME_COUNT = 3;

extern ID3D12Device        *gDevice;
extern IDXGISwapChain3     *gSwapChain;
extern ID3D12CommandQueue  *gCommandQueue;
extern CommandContext      *gCommand;

extern DescriptorHeap      *gRenderTargetHeap;
extern DescriptorHeap      *gDepthStencilHeap;

extern RenderTargetBuffer  *gRenderTarget[FRAME_COUNT];
extern DepthStencilBuffer  *gDepthStencil;

// feature abilities
extern bool                 gTypedUAVLoadSupport_R11G11B10_FLOAT;
extern bool                 gTypedUAVLoadSupport_R16G16B16A16_FLOAT;
extern bool                 gHDROutputSupport;

extern void Initialize(HWND hwnd);
extern void Terminate(void);

extern ID3D12Resource * GetRenderTarget(uint32_t frameIndex);
extern const D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle(uint32_t frameIndex);
extern const D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilViewHandle(void);

}