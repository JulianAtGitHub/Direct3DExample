#pragma once

#include <random>
#include "Shaders/PathTrace/SharedTypes.h"

class PtExample : public Utils::AnExample {
public:
    PtExample(void);
    virtual ~PtExample(void);

    virtual void Init(HWND hwnd);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

    virtual void OnKeyDown(uint8_t key);
    virtual void OnKeyUp(uint8_t key);
    virtual void OnChar(uint16_t cha);
    virtual void OnMouseLButtonDown(int64_t pos);
    virtual void OnMouseLButtonUp(int64_t pos);
    virtual void OnMouseRButtonDown(int64_t pos);
    virtual void OnMouseRButtonUp(int64_t pos);
    virtual void OnMouseMButtonDown(int64_t pos);
    virtual void OnMouseMButtonUp(int64_t pos);
    virtual void OnMouseMove(int64_t pos);
    virtual void OnMouseWheel(uint64_t param);

private:
    void InitScene(void);
    void CreateRootSignature(void);
    void CreateRayTracingPipelineState(void);
    void BuildGeometry(void);
    void BuildAccelerationStructure(void);
    void BuildShaderTables(void);
    void CreateRaytracingOutput(void);
    void PrepareScreenPass(void);

    void UpdateGUI(float dt);
    void Profiling(void);

    enum GlobalRootSignatureParams {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        AppSettingsSlot,
        SceneConstantsSlot,
        CameraConstantsSlot,
        BuffersSlot,
        EnvTexturesSlot,
        TexturesSlot,
        SamplerSlot,
        SlotCount
    };

    enum SPRootSignatureParams {
        SPTextureSlot = 0,
        SPSamplerSlot,
        SPSlotCount
    };

    Utils::Timer    mTimer;
    Utils::Camera  *mCamera;
    // jitter camera
    std::mt19937                            mRang;
    std::uniform_real_distribution<float>   mRangDist;

    float           mSpeedX;
    float           mSpeedZ;
    bool            mIsRotating;
    int64_t         mLastMousePos;
    int64_t         mCurrentMousePos;

    uint32_t        mWidth;
    uint32_t        mHeight;
    uint32_t        mAccumCount;
    uint32_t        mCurrentFrame;
    uint64_t        mFenceValues[Render::FRAME_COUNT];
    bool            mEnableScreenPass;
    Utils::Scene   *mScene;

    Render::RootSignature              *mGlobalRootSignature;
    Render::RootSignature              *mLocalRootSignature;
    Render::RayTracingState            *mRayTracingState;
    Render::DescriptorHeap             *mDescriptorHeap;
    Render::GPUBuffer                  *mVertices;
    Render::GPUBuffer                  *mIndices;
    Render::GPUBuffer                  *mGeometries;
    Render::GPUBuffer                  *mMaterials;
    Render::GPUBuffer                  *mLights;
    Render::PixelBuffer                *mRaytracingOutput;
    Render::DescriptorHeap             *mSamplerHeap;
    Render::Sampler                    *mSampler;
    Render::PixelBuffer                *mEnvTexture;
    std::vector<Render::PixelBuffer *>  mTextures;

    AppSettings                 mSettings;
    SceneConstants              mSceneConsts;
    CameraConstants             mCameraConsts;
    Render::ConstantBuffer     *mSettingsCB;
    Render::ConstantBuffer     *mSceneCB;
    Render::ConstantBuffer     *mCameraCB;

    typedef Render::BottomLevelAccelerationStructure BLAS;
    typedef Render::TopLevelAccelerationStructure TLAS;
    std::vector<BLAS *>         mBLASes;
    TLAS                       *mTLAS;

    // screen pass
    Render::RootSignature      *mSPRootSignature;
    Render::GraphicsState      *mSPGraphicsState;
    Render::GPUBuffer          *mSPVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW    mSPVertexBufferView;

    Utils::GUILayer            *mGUI;

    // profile
    Utils::Timer    mProfileTimer;
    uint32_t        mFrameCount;
    uint32_t        mFrameStart;
    size_t          mVertexCount;
    size_t          mIndexCount;
};
