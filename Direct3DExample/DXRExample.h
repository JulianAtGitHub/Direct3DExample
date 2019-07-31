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
    void CreateDescriptorHeap(void);
    void BuildGeometry(void);
    void BuildAccelerationStructure(void);
    void CreateConstantBuffer(void);
    void BuildShaderTables(void);
    void CreateRaytracingOutput(void);

    struct SceneConstantBuffer {
        XMMATRIX projectionToWorld;
        XMVECTOR cameraPosition;
        XMVECTOR lightDirection;
        XMVECTOR lightAmbientColor;
        XMVECTOR lightDiffuseColor;
    };

    struct MeshConstantBuffer {
        XMFLOAT4 albedo[2];
        XMUINT2 offset;
    };

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT3 normal;
    };

    enum GlobalRootSignatureParams {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        SceneConstantSlot,
        MeshConstantSlot,
        VertexBuffersSlot,
        Count
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
    uint32_t                    mCurrentFrame;
    uint64_t                    mFenceValues[Render::FRAME_COUNT];
    SceneConstantBuffer         mSceneConstBuf[Render::FRAME_COUNT];
    MeshConstantBuffer          mMeshConstBuf;

    Render::RootSignature      *mGlobalRootSignature;
    Render::RootSignature      *mLocalRootSignature;
    Render::RayTracingState    *mRayTracingState;
    Render::DescriptorHeap     *mDescriptorHeap;
    Render::GPUBuffer          *mVertices;
    Render::GPUBuffer          *mIndices;
    Render::ConstantBuffer     *mSceneConstantBuffer;
    Render::UploadBuffer       *mMeshConstantBuffer;
    Render::PixelBuffer        *mRaytracingOutput;

    Render::BottomLevelAccelerationStructure   *mBLASCube;
    Render::BottomLevelAccelerationStructure   *mBLASPlane;
    Render::TopLevelAccelerationStructure      *mTLAS;
};
