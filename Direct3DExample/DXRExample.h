#pragma once

#include "Example.h"
#include "Core/Render/RenderCore.h"

namespace Render {
    class RootSignature;
    class DescriptorHeap;
    class RayTracingState;
    class GPUBuffer;
    class UploadBuffer;
    class ConstantBuffer;
    class PixelBuffer;
    class BottomLevelAccelerationStructure;
    class TopLevelAccelerationStructure;
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
    Render::UploadBuffer       *mShaderTable;
    D3D12_DISPATCH_RAYS_DESC    mRaysDesc;

    Render::BottomLevelAccelerationStructure   *mBLASCube;
    Render::BottomLevelAccelerationStructure   *mBLASPlane;
    Render::TopLevelAccelerationStructure      *mTLAS;
};
