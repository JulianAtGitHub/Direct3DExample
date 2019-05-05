#pragma once

#include "DescriptorHeap.h"
#include "ColorBuffer.h"
#include "DepthStencilBuffer.h"

namespace Render {

class Context {
public:
    static const uint32_t FRAME_COUNT = 3;

    Context(HWND hwnd);
    ~Context(void);

    INLINE ID3D12Resource * GetRenderTarget(uint32_t frameIndex) { assert(frameIndex < FRAME_COUNT); return mRenderTarget[frameIndex]->GetResource();}
    INLINE const D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle(uint32_t frameIndex) const { assert(frameIndex < FRAME_COUNT); return mRenderTarget[frameIndex]->GetHandle().cpu; }
    INLINE const D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilViewHandle(void) const { return mDepthStencil->GetHandle().cpu; }

    // TODO: remove these functions
    INLINE ID3D12Device * Device() { return mDevice; }
    INLINE IDXGISwapChain3 * SwapChain() { return mSwapChain; }
    INLINE ID3D12CommandQueue * CommandQueue() { return mCommandQueue; }

private:
    void Initialize(HWND hwnd);

    ID3D12Device           *mDevice;
    IDXGISwapChain3        *mSwapChain;
    ID3D12CommandQueue     *mCommandQueue;

    DescriptorHeap         *mRenderTargetHeap;
    DescriptorHeap         *mDepthStencilHeap;

    ColorBuffer            *mRenderTarget[FRAME_COUNT];
    DepthStencilBuffer     *mDepthStencil;

    // feature abilities
    bool mTypedUAVLoadSupport_R11G11B10_FLOAT;
    bool mTypedUAVLoadSupport_R16G16B16A16_FLOAT;
    bool mHDROutputSupport;
};

}