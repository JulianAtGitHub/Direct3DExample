#pragma once

class D3DExample : public Utils::AnExample {
public:
    D3DExample(void);
    virtual ~D3DExample(void);

    virtual void Init(HWND hwnd);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

    virtual void OnKeyDown(uint8_t key);
    virtual void OnKeyUp(uint8_t key);
    virtual void OnMouseLButtonDown(int64_t pos);
    virtual void OnMouseLButtonUp(int64_t pos);
    virtual void OnMouseMove(int64_t pos);

private:
    void LoadPipeline(void);
    void LoadAssets(void);
    void PopulateCommandList(void);
    void MoveToNextFrame(void);

    Render::RootSignature *mRootSignature;
    Render::GraphicsState *mGraphicsState;

    Render::GPUBuffer *mVertexIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    Render::ConstantBuffer *mConstBuffer;

    Render::DescriptorHeap *mShaderResourceHeap;
    std::vector<Render::PixelBuffer *> mTextures;

    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler *mSampler;

    uint64_t mFenceValues[Render::FRAME_COUNT];
    uint32_t mCurrentFrame;

    uint32_t mWidth;
    uint32_t mHeight;

    float   mSpeedX;
    float   mSpeedZ;
    bool    mIsRotating;
    int64_t mLastMousePos;
    int64_t mCurrentMousePos;

    Utils::Timer    mTimer;
    Utils::Camera  *mCamera;
    Utils::Scene   *mScene;
};
