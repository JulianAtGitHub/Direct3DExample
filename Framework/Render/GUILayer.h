#pragma once

struct ImGuiContext;

namespace Render {

class PixelBuffer;
class Sampler;
class DescriptorHeap;
class RootSignature;
class GraphicsState;
class GPUBuffer;
class ConstantBuffer;
class RenderTargetBuffer;

class GUILayer {
public:
    GUILayer(HWND hwnd, uint32_t width, uint32_t height);
    ~GUILayer(void);

    void BeginFrame(float deltaTime);
    void EndFrame(uint32_t frameIdx);
    void Draw(uint32_t frameIdx, RenderTargetBuffer *renderTarget);

    // Input
    void OnKeyDown(uint8_t key);
    void OnKeyUp(uint8_t key);
    void OnChar(uint16_t cha);
    void OnMouseLButtonDown(void);
    void OnMouseLButtonUp(void);
    void OnMouseRButtonDown(void);
    void OnMouseRButtonUp(void);
    void OnMouseMButtonDown(void);
    void OnMouseMButtonUp(void);
    void OnMouseMove(int64_t pos);
    void OnMouseWheel(uint64_t param);

private:
    void Initialize(HWND hwnd);
    void Destroy(void);

    uint32_t        mWidth;
    uint32_t        mHeight;

    ImGuiContext   *mContext;
    PixelBuffer    *mFontTexture;
    Sampler        *mSampler;
    DescriptorHeap *mResourceHeap;
    DescriptorHeap *mSamplerHeap;
    RootSignature  *mRootSignature;
    GraphicsState  *mGraphicsState;

    ConstantBuffer *mConstBuffer;
    GPUBuffer      *mVertexBuffer; // vertices & indices
    D3D12_VERTEX_BUFFER_VIEW    mVertexView;
    D3D12_INDEX_BUFFER_VIEW     mIndexView;

    const static size_t VERTEX_BUFFER_SIZE    = 0x200000; // 2MB
};

}
