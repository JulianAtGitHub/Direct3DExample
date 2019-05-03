#pragma once

namespace Render {

class Context {
public:
    static const uint32_t FRAME_COUNT = 3;

    Context(HWND hwnd);
    ~Context(void);

    // TODO: remove these functions
    inline ID3D12Device * Device() { return mDevice; }
    inline IDXGISwapChain3 * SwapChain() { return mSwapChain; }
    inline ID3D12CommandQueue * CommandQueue() { return mCommandQueue; }

private:
    void Initialize(HWND hwnd);

    ID3D12Device           *mDevice;
    IDXGISwapChain3        *mSwapChain;
    ID3D12CommandQueue     *mCommandQueue;

    // feature abilities
    bool mTypedUAVLoadSupport_R11G11B10_FLOAT;
    bool mTypedUAVLoadSupport_R16G16B16A16_FLOAT;
    bool mHDROutputSupport;
};

}