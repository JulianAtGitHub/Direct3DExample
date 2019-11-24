#pragma once

class PbrDrawable {
public:
    PbrDrawable(Utils::Scene *scene);
    ~PbrDrawable(void);

    INLINE Render::ConstantBuffer * GetConstBuffer(void) const { return mConstBuffer; }
    INLINE const D3D12_VERTEX_BUFFER_VIEW & GetVertexBufferView(void) const { return mVertexBufferView; }
    INLINE const D3D12_INDEX_BUFFER_VIEW & GetIndexBufferView(void) const { return mIndexBufferView; }
    INLINE const std::vector<Utils::Scene::Shape> & GetShapes(void) const { return mShapes; }

    void Update(uint32_t currentFrame, const XMFLOAT4X4 &mvp);

private:
    void Initialize(Utils::Scene *scene);
    void Destroy(void);

    Render::ConstantBuffer             *mConstBuffer;
    Render::DescriptorHeap             *mTextureHeap;
    std::vector<Render::PixelBuffer *>  mTextures;
    Render::GPUBuffer                  *mVertexBuffer;
    Render::GPUBuffer                  *mIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW            mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW             mIndexBufferView;
    std::vector<Utils::Scene::Shape>    mShapes;
};
