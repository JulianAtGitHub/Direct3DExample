#pragma once

#include "Example.h"

#include <random>

class DXRExample : public Example {
public:
    DXRExample(HWND hwnd);
    virtual ~DXRExample(void);

    virtual void Init(void);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

    virtual void OnKeyDown(uint8_t key);
    virtual void OnKeyUp(uint8_t key);
    virtual void OnMouseLButtonDown(int64_t pos);
    virtual void OnMouseLButtonUp(int64_t pos);
    virtual void OnMouseMove(int64_t pos);

private:
    void InitScene(void);
    void CreateRootSignature(void);
    void CreateRayTracingPipelineState(void);
    void BuildGeometry(void);
    void BuildAccelerationStructure(void);
    void BuildShaderTables(void);
    void CreateRaytracingOutput(void);
    void PrepareScreenPass(void);

    void Profiling(void);

    struct AppSettings {
        uint32_t enableAccumulate;
        uint32_t enableJitterCamera;
        uint32_t enableLensCamera;
        uint32_t enableEnvironmentMap;
        uint32_t enableIndirectLight;
    };

    struct Geometry {
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t isOpacity;
        uint32_t reserve;
        XMUINT4 texInfo;  // x: diffuse, y: metallic, z:roughness, w: normal
    };

    enum LightType {
        DirectLight = 0,
        PointLight,
        SpotLight,
    };

    struct Light {
        uint32_t  type; // LightType
        float     openAngle;
        float     penumbraAngle;
        float     cosOpenAngle;
        XMFLOAT3  position;
        XMFLOAT3  direction;
        XMFLOAT3  intensity;
    };

    struct CameraConstants {
        XMFLOAT4 position;
        XMFLOAT4 u;
        XMFLOAT4 v;
        XMFLOAT4 w;
        XMFLOAT2 jitter;
        float    lensRadius;
        float    focalLength;
    };

    struct SceneConstants {
        XMFLOAT4 bgColor;
        uint32_t lightCount;
        uint32_t frameSeed;
        uint32_t accumCount;
        uint32_t maxRayDepth;
        uint32_t sampleCount;
    };

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
    uint32_t        mLightCount;
    uint32_t        mAccumCount;
    uint32_t        mMaxRayDepth;
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

    // profile
    Utils::Timer    mProfileTimer;
    uint32_t        mFrameCount;
    uint32_t        mFrameStart;
};
