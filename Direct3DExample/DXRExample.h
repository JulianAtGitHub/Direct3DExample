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

    struct AppSettings {
        uint32_t enableAccumulate;
        uint32_t enableJitterCamera;
        uint32_t enableLensCamera;
    };

    struct CameraConstants {
        XMFLOAT4 pos;
        XMFLOAT4 u;
        XMFLOAT4 v;
        XMFLOAT4 w;
        XMFLOAT2 jitter;
        float    lensRadius;
        float    focalLength;
    };

    struct SceneConstants {
        XMFLOAT4 bgColor;
        uint32_t frameCount;
        uint32_t accumCount;
        float    aoRadius;
    };

    struct Geometry {
        XMUINT4 indexInfo; // x: index offset, y: index count;
        XMUINT4 texInfo;  // x: diffuse, y: specular, z: normal
    };

    enum GlobalRootSignatureParams {
        OutputViewSlot = 0,
        OutputColorSlot,
        AccelerationStructureSlot,
        AppSettingsSlot,
        SceneConstantsSlot,
        CameraConstantsSlot,
        VertexBuffersSlot,
        TexturesSlot,
        SamplerSlot,
        SlotCount
    };

    CTimer                      mTimer;
    Utils::Camera              *mCamera;
    // jitter camera
    std::mt19937                            mRang;
    std::uniform_real_distribution<float>   mRangDist;

    float                       mSpeedX;
    float                       mSpeedZ;
    bool                        mIsRotating;
    int64_t                     mLastMousePos;
    int64_t                     mCurrentMousePos;

    uint32_t                    mWidth;
    uint32_t                    mHeight;
    uint32_t                    mFrameCount;
    uint32_t                    mAccumCount;
    uint32_t                    mCurrentFrame;
    uint64_t                    mFenceValues[Render::FRAME_COUNT];
    Utils::Scene               *mScene;

    Render::RootSignature      *mGlobalRootSignature;
    Render::RootSignature      *mLocalRootSignature;
    Render::RayTracingState    *mRayTracingState;
    Render::DescriptorHeap     *mDescriptorHeap;
    Render::GPUBuffer          *mVertices;
    Render::GPUBuffer          *mIndices;
    Render::GPUBuffer          *mGeometries;
    Render::PixelBuffer        *mRaytracingOutput;
    Render::PixelBuffer        *mDisplayColor;
    Render::DescriptorHeap     *mSamplerHeap;
    Render::Sampler            *mSampler;
    CList<Render::PixelBuffer*> mTextures;

    AppSettings                 mSettings;
    SceneConstants              mSceneConsts;
    CameraConstants             mCameraConsts;
    Render::ConstantBuffer     *mSettingsCB;
    Render::ConstantBuffer     *mSceneCB;
    Render::ConstantBuffer     *mCameraCB;
    Render::UploadBuffer       *mMeshCB;

    typedef Render::BottomLevelAccelerationStructure BLAS;
    typedef Render::TopLevelAccelerationStructure TLAS;
    CList<BLAS *>               mBLASes;
    TLAS                       *mTLAS;
};
