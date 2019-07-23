#pragma once

namespace Render {

class RootSignature;

class RayTracingState {
public:
    RayTracingState(uint32_t count);
    ~RayTracingState(void);

    INLINE ID3D12StateObject * Get(void) const { return mState; }

    int32_t AddStateObjectConfig(D3D12_STATE_OBJECT_FLAGS flags);
    int32_t AddGlobalRootSignature(RootSignature *rs);
    int32_t AddLocalRootSignature(RootSignature *rs);
    int32_t AddNodeMask(uint32_t mask);
    int32_t AddDXILLibrary(const char *shaderFile, const wchar_t *functions[], uint32_t funcCount);
    int32_t AddSubObjectToExportsAssociation(uint32_t subObjIndex, const wchar_t *exports[], uint32_t exportCount);
    int32_t AddDXILSubObjectToExportsAssociation(const wchar_t *subObj, const wchar_t *exports[], uint32_t exportCount);
    int32_t AddRayTracingShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize);
    int32_t AddRayTracingPipelineConfig(uint32_t maxTraceDepth);
    int32_t AddHitGroup(const wchar_t *hitGroup, const wchar_t *closetHit = nullptr, const wchar_t *anyHit = nullptr, const wchar_t *intersection = nullptr, D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES);

    void Create(void);

private:
    int32_t AddSubObject(D3D12_STATE_SUBOBJECT_TYPE type, const void *object);
    void CleanupSubObjects(void);

    void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC &desc);

    CList<D3D12_STATE_SUBOBJECT> mSubObjects;
    ID3D12StateObject           *mState;
};

}
