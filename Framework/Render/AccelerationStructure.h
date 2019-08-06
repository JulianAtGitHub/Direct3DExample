#pragma once

namespace Render {

class GPUResource;
class GPUBuffer;
class UploadBuffer;

class AccelerationStructure {
public:
    AccelerationStructure(void);
    virtual ~AccelerationStructure(void);

    INLINE const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS & GetInputs(void) const { return mInputs; }
    INLINE GPUBuffer * GetResult(void) const { return mResultData; }
    INLINE GPUBuffer * GetScratch(void) const { return mScratchData; }

protected:
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS    mInputs;
    GPUBuffer  *mResultData;
    GPUBuffer  *mScratchData;
};

class BottomLevelAccelerationStructure : public AccelerationStructure {
public:
    BottomLevelAccelerationStructure(void);
    virtual ~BottomLevelAccelerationStructure(void);

    void AddTriangles(GPUBuffer *indices, uint64_t indexOffset, uint32_t indexCount, bool is16Bit,
                      GPUBuffer *vertices, uint64_t vertexOffset, uint32_t vertexCount, uint64_t stride, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT,
                      D3D12_RAYTRACING_GEOMETRY_FLAGS flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE);

    bool PreBuild(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);
    void PostBuild(void);

private:
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>   mGeometryDescs;
};

class TopLevelAccelerationStructure : public AccelerationStructure {
public:
    TopLevelAccelerationStructure(void);
    virtual ~TopLevelAccelerationStructure(void);

    void AddInstance(BottomLevelAccelerationStructure *blas, uint32_t id, uint32_t hitGroupIdx, XMMATRIX &transform, uint8_t idMask = 0xFF, uint8_t flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE);

    bool PreBuild(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);
    void PostBuild(void);

private:
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC>   mInstanceDescs;
    UploadBuffer   *mInstances;
};

}
