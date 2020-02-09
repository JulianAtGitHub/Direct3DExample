#pragma once

#include "Shaders/types.pbr.h"

class PbrDrawable {
public:
    PbrDrawable(void);
    ~PbrDrawable(void);

    void Initialize(const std::string &name, Utils::Scene *scene, 
                    Render::GPUBuffer *lightBuffer, uint32_t lightCount, 
                    Render::PixelBuffer **envTexs, uint32_t envTexCount);
    void Update(uint32_t currentFrame, Utils::Camera &camera, const SettingsCB &settings, const MatValuesCB &matValues);

    INLINE const std::string & GetName(void) const { return mName; }
    INLINE Render::DescriptorHeap *GetResourceHeap(void) const { return mResourceHeap; }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetSettingsCB(uint32_t currentFrame) { return mSettingsCB->GetGPUAddress(0, currentFrame); }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetTransformCB(uint32_t currentFrame) const { return mTransformCB->GetGPUAddress(0, currentFrame); }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetMatValuesCB(uint32_t shapeIdx, uint32_t currentFrame) const { return mMatValuesCB->GetGPUAddress(shapeIdx, currentFrame); }
    INLINE const D3D12_VERTEX_BUFFER_VIEW & GetVertexBufferView(void) const { return mVertexBufferView; }
    INLINE const D3D12_INDEX_BUFFER_VIEW & GetIndexBufferView(void) const { return mIndexBufferView; }
    INLINE const std::vector<Utils::Scene::Shape> & GetShapes(void) const { return mShapes; }
    INLINE Render::DescriptorHandle GetLightBufferHandle(void) { return mResourceHeap->GetHandle(LIGHT_HEAP_INDEX); }
    INLINE Render::DescriptorHandle GetMaterialBufferHandle(void) { return mResourceHeap->GetHandle(MAT_HEAP_INDEX); }
    INLINE Render::DescriptorHandle GetEnvTexsHandle(void) { return mResourceHeap->GetHandle(ENV_HEAP_INDEX); }
    INLINE Render::DescriptorHandle GetMatTexsHandle(void) { return mResourceHeap->GetHandle(mMatTexsOffset); }

private:
    static constexpr uint32_t LIGHT_HEAP_INDEX = 0;
    static constexpr uint32_t MAT_HEAP_INDEX = 1;
    static constexpr uint32_t ENV_HEAP_INDEX = 2;

    void Destroy(void);

    std::string                         mName;
    Render::DescriptorHeap             *mResourceHeap;
    Render::ConstantBuffer             *mSettingsCB;
    Render::ConstantBuffer             *mTransformCB;
    Render::ConstantBuffer             *mMatValuesCB;
    Render::GPUBuffer                  *mMaterialBuffer;
    std::vector<Render::PixelBuffer *>  mTextures;
    Render::GPUBuffer                  *mVertexBuffer;
    Render::GPUBuffer                  *mIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW            mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW             mIndexBufferView;
    std::vector<Utils::Scene::Shape>    mShapes;
    uint32_t                            mMatTexsOffset;
};
