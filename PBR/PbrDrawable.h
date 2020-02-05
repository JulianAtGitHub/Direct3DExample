#pragma once

#include "Shaders/types.pbr.h"

class PbrDrawable {
public:
    PbrDrawable(void);
    ~PbrDrawable(void);

    void Initialize(const std::string &name, Utils::Scene *scene, Render::DescriptorHeap *texHeap);
    void Update(uint32_t currentFrame, Utils::Camera &camera, const SettingsCB &settings);

    INLINE const std::string & GetName(void) const { return mName; }
    INLINE MaterialCB & GetMaterial(void) { return mMaterial; }
    INLINE uint32_t GetMatTexOffset(void) const { return mMatTexOffset; }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetSettingsCB(uint32_t currentFrame) { return mSettingsCB->GetGPUAddress(0, currentFrame); }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetTransformCB(uint32_t currentFrame) const { return mTransformCB->GetGPUAddress(0, currentFrame); }
    INLINE D3D12_GPU_VIRTUAL_ADDRESS GetMaterialCB(uint32_t currentFrame) const { return mMaterialCB->GetGPUAddress(0, currentFrame); }
    INLINE const D3D12_VERTEX_BUFFER_VIEW & GetVertexBufferView(void) const { return mVertexBufferView; }
    INLINE const D3D12_INDEX_BUFFER_VIEW & GetIndexBufferView(void) const { return mIndexBufferView; }
    INLINE const std::vector<Utils::Scene::Shape> & GetShapes(void) const { return mShapes; }

private:
    void Destroy(void);

    std::string                         mName;
    MaterialCB                          mMaterial;
    uint32_t                            mMatTexOffset;
    Render::ConstantBuffer             *mSettingsCB;
    Render::ConstantBuffer             *mTransformCB;
    Render::ConstantBuffer             *mMaterialCB;
    std::vector<Render::PixelBuffer *>  mTextures;
    Render::GPUBuffer                  *mVertexBuffer;
    Render::GPUBuffer                  *mIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW            mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW             mIndexBufferView;
    std::vector<Utils::Scene::Shape>    mShapes;
};
