#pragma once

class SkyboxPass;

class PbrExample : public Utils::AnExample {
public:
    PbrExample(void);
    virtual ~PbrExample(void);

    virtual void Init(HWND hwnd);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

private:
    static std::string WindowTitle;

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mCurrentFrame;
    uint64_t mFenceValues[Render::FRAME_COUNT];

    Utils::Camera  *mCamera;
    SkyboxPass     *mSkybox;

    Render::RootSignature      *mRootSignature;
    Render::GraphicsState      *mGraphicsState;
    Render::ConstantBuffer     *mConstBuffer;
    Render::GPUBuffer          *mVertexBuffer;
    Render::GPUBuffer          *mIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW    mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW     mIndexBufferView;

    std::vector<Utils::Scene::Shape> mShapes;
    
};
