#include "stdafx.h"
#include "PbrDrawable.h"


PbrDrawable::PbrDrawable(Utils::Scene *scene)
: mConstBuffer(nullptr)
, mTextureHeap(nullptr)
, mVertexBuffer(nullptr)
, mIndexBuffer(nullptr)
{
    Initialize(scene);
}

PbrDrawable::~PbrDrawable(void) {
    Destroy();
}

void PbrDrawable::Initialize(Utils::Scene *scene) {
    ASSERT_PRINT(scene);
    if (!scene) {
        return;
    }

    mConstBuffer = new Render::ConstantBuffer(sizeof(XMFLOAT4X4), 1);

    size_t verticesSize = scene->mVertices.size() * sizeof(Utils::Scene::Vertex);
    size_t indicesSize = scene->mIndices.size() * sizeof(uint32_t);

    mVertexBuffer = new Render::GPUBuffer(verticesSize);
    mIndexBuffer = new Render::GPUBuffer(indicesSize);

    mVertexBufferView = mVertexBuffer->FillVertexBufferView(0, static_cast<uint32_t>(verticesSize), sizeof(Utils::Scene::Vertex));
    mIndexBufferView = mIndexBuffer->FillIndexBufferView(0, static_cast<uint32_t>(indicesSize), false);

    if (scene->mImages.size() > 0) {
        mTextureHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(scene->mImages.size()));
        mTextures.reserve(scene->mImages.size());
        for (auto image : scene->mImages) {
            Render::PixelBuffer *texture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), image->GetMipLevels(), image->GetDXGIFormat(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            texture->CreateSRV(mTextureHeap->Allocate());
            mTextures.push_back(texture);
        }
    }

    Render::gCommand->Begin();

    Render::gCommand->UploadBuffer(mVertexBuffer, 0, scene->mVertices.data(), verticesSize);
    Render::gCommand->UploadBuffer(mIndexBuffer, 0, scene->mIndices.data(), indicesSize);
    for (uint32_t i = 0; i < mTextures.size(); ++i) {
        Render::gCommand->UploadTexture(mTextures[i], scene->mImages[i]->GetPixels());
    }

    Render::gCommand->End(true);

    for (auto texture : mTextures) {
        Utils::gMipsGener->Dispatch(texture);
    }

    mShapes = scene->mShapes;
}

void PbrDrawable::Destroy(void) {
    for (auto texture : mTextures) {
        delete texture;
    }
    mTextures.clear();
    DeleteAndSetNull(mTextureHeap);
    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mIndexBuffer);
    DeleteAndSetNull(mVertexBuffer);
}

void PbrDrawable::Update(uint32_t currentFrame, const XMFLOAT4X4 &mvp) {
    mConstBuffer->CopyData(&mvp, sizeof(XMFLOAT4X4), 0, currentFrame);
}

