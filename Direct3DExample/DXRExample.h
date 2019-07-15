#pragma once

#include "Example.h"
#include "Core/Render/RenderCore.h"

namespace Render {
    class DescriptorHeap;
    class GPUBuffer;
    class UploadBuffer;
    class ConstantBuffer;
    class PixelBuffer;
}

class DXRExample : public Example {
public:
    DXRExample(HWND hwnd);
    virtual ~DXRExample(void);

    virtual void Init(void);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

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

    void UpdateCameraMatrices();

    struct SceneConstantBuffer {
        XMMATRIX projectionToWorld;
        XMVECTOR cameraPosition;
        XMVECTOR lightPosition;
        XMVECTOR lightAmbientColor;
        XMVECTOR lightDiffuseColor;
    };

    struct CubeConstantBuffer {
        XMFLOAT4 albedo;
    };

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT3 normal;
    };

    uint32_t                mWidth;
    uint32_t                mHeight;
    uint32_t                mCurrentFrame;
    uint64_t                mFenceValues[Render::FRAME_COUNT];
    SceneConstantBuffer     mSceneConstBuf[Render::FRAME_COUNT];
    CubeConstantBuffer      mCubeConstBuf;

    ID3D12RootSignature    *mGlobalRootSignature;
    ID3D12RootSignature    *mLocalRootSignature;
    ID3D12StateObject      *mStateObject;
    Render::DescriptorHeap *mDescriptorHeap;
    Render::GPUBuffer      *mVertexBuffer;
    Render::GPUBuffer      *mIndexBuffer;
    Render::GPUBuffer      *mBottomLevelAccelerationStructure;
    Render::GPUBuffer      *mTopLevelAccelerationStructure;
    Render::ConstantBuffer *mSceneConstantBuffer;
    Render::UploadBuffer   *mRayGenShaderTable;
    Render::UploadBuffer   *mMissShaderTable;
    Render::UploadBuffer   *mHitGroupShaderTable;
    Render::PixelBuffer    *mRaytracingOutput;
};
