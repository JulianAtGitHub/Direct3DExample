#include "stdafx.h"
#include "PbrDrawable.h"


PbrDrawable::PbrDrawable(void)
: mMatTexOffset(0)
, mSettingsCB(nullptr)
, mTransformCB(nullptr)
, mMaterialCB(nullptr)
, mVertexBuffer(nullptr)
, mIndexBuffer(nullptr)
{

}

PbrDrawable::~PbrDrawable(void) {
    Destroy();
}

void PbrDrawable::Initialize(const std::string &name, Utils::Scene *scene, Render::DescriptorHeap *texHeap) {
    ASSERT_PRINT(scene && texHeap);
    if (!scene || !texHeap) {
        return;
    }

    mName = name;

    mMaterial = { {1.0f, 1.0f, 1.0f }, 0.5f, 0.5f, 1.0f };

    mSettingsCB = new Render::ConstantBuffer(sizeof(SettingsCB), 1);
    mTransformCB = new Render::ConstantBuffer(sizeof(TransformCB), 1);
    mMaterialCB = new Render::ConstantBuffer(sizeof(MaterialCB), 1);

    size_t verticesSize = scene->mVertices.size() * sizeof(Utils::Scene::Vertex);
    size_t indicesSize = scene->mIndices.size() * sizeof(uint32_t);

    mVertexBuffer = new Render::GPUBuffer(verticesSize);
    mIndexBuffer = new Render::GPUBuffer(indicesSize);

    mVertexBufferView = mVertexBuffer->FillVertexBufferView(0, static_cast<uint32_t>(verticesSize), sizeof(Utils::Scene::Vertex));
    mIndexBufferView = mIndexBuffer->FillIndexBufferView(0, static_cast<uint32_t>(indicesSize), false);

    mMatTexOffset = texHeap->GetUsedCount();

    if (scene->mImages.size() > 0) {
        mTextures.reserve(scene->mImages.size());
        for (auto image : scene->mImages) {
            Render::PixelBuffer *texture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), image->GetMipLevels(), image->GetDXGIFormat(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            texture->CreateSRV(texHeap->Allocate());
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
    DeleteAndSetNull(mSettingsCB);
    DeleteAndSetNull(mTransformCB);
    DeleteAndSetNull(mMaterialCB);
    for (auto texture : mTextures) {
        delete texture;
    }
    mTextures.clear();
    DeleteAndSetNull(mIndexBuffer);
    DeleteAndSetNull(mVertexBuffer);
}

void PbrDrawable::Update(uint32_t currentFrame, Utils::Camera &camera, const SettingsCB &settings) {
    XMMATRIX model = XMMatrixIdentity();
    XMMATRIX view = camera.GetViewMatrix();
    XMMATRIX proj = camera.GetProjectMatrix();

    TransformCB transform;
    XMStoreFloat4x4(&(transform.model), model);
    XMStoreFloat4x4(&(transform.mvp), XMMatrixMultiply(XMMatrixMultiply(model, view), proj));
    XMStoreFloat4(&(transform.cameraPos), camera.GetPosition());

    mSettingsCB->CopyData(&settings, sizeof(SettingsCB), 0, currentFrame);
    mTransformCB->CopyData(&transform, sizeof(TransformCB), 0, currentFrame);
    mMaterialCB->CopyData(&mMaterial, sizeof(MaterialCB), 0, currentFrame);
}

