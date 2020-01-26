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
}

namespace Utils {

class GUILayer {
public:
    GUILayer(HWND hwnd, uint32_t width, uint32_t height);
    ~GUILayer(void);

    bool AddImage(Render::PixelBuffer *image);
    bool SetImageMipLevel(Render::PixelBuffer *image, uint32_t mipLevel);

    void BeginFrame(float deltaTime);
    void EndFrame(uint32_t frameIdx);
    void Draw(uint32_t frameIdx, Render::RenderTargetBuffer *renderTarget);

    bool IsHovered(void);

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
    const static uint32_t   RESOURCE_COUNT      = 32;
    const static size_t     VERTEX_BUFFER_SIZE  = 0x200000; // 2MB

    void Initialize(HWND hwnd);
    void Destroy(void);

    uint32_t        mWidth;
    uint32_t        mHeight;

    ImGuiContext               *mContext;
    Render::PixelBuffer        *mFontTexture;
    Render::Sampler            *mSampler;
    Render::DescriptorHeap     *mResourceHeap;
    Render::DescriptorHeap     *mSamplerHeap;
    Render::RootSignature      *mRootSignature;
    Render::GraphicsState      *mGraphicsState;

    Render::ConstantBuffer     *mConstBuffer;
    Render::GPUBuffer          *mVertexBuffer[Render::FRAME_COUNT]; // vertices & indices
    D3D12_VERTEX_BUFFER_VIEW    mVertexView;
    D3D12_INDEX_BUFFER_VIEW     mIndexView;

    struct ResourceInfo {
        uint32_t handlerIndex;
        uint32_t mipLevel;
    };
    std::map<const void *, ResourceInfo> mResourceMap;
};

}
