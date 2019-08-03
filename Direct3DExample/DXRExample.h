#pragma once

#include "Example.h"

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

    struct SceneConstants {
        XMVECTOR cameraPos;
        XMVECTOR cameraU;
        XMVECTOR cameraV;
        XMVECTOR cameraW;
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
        SceneConstantsSlot,
        VertexBuffersSlot,
        TexturesSlot,
        SamplerSlot,
        SlotCount
    };

    CTimer                      mTimer;
    Utils::Camera              *mCamera;
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
    SceneConstants              mSceneConsts[Render::FRAME_COUNT];
    Utils::Scene               *mScene;

    Render::RootSignature      *mGlobalRootSignature;
    Render::RootSignature      *mLocalRootSignature;
    Render::RayTracingState    *mRayTracingState;
    Render::DescriptorHeap     *mDescriptorHeap;
    Render::GPUBuffer          *mVertices;
    Render::GPUBuffer          *mIndices;
    Render::GPUBuffer          *mGeometries;
    Render::ConstantBuffer     *mSceneConstantBuffer;
    Render::UploadBuffer       *mMeshConstantBuffer;
    Render::PixelBuffer        *mRaytracingOutput;
    Render::PixelBuffer        *mDisplayColor;
    Render::DescriptorHeap     *mSamplerHeap;
    Render::Sampler            *mSampler;
    CList<Render::PixelBuffer*> mTextures;

    typedef Render::BottomLevelAccelerationStructure BLAS;
    typedef Render::TopLevelAccelerationStructure TLAS;
    CList<BLAS *>               mBLASes;
    TLAS                       *mTLAS;
};
