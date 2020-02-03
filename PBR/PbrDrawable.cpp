#include "stdafx.h"
#include "PbrDrawable.h"


PbrDrawable::PbrDrawable(void)
: mSettingsCB(nullptr)
, mTransformCB(nullptr)
, mMaterialCB(nullptr)
, mResourceHeap(nullptr)
, mVertexBuffer(nullptr)
, mIndexBuffer(nullptr)
{

}

PbrDrawable::~PbrDrawable(void) {
    Destroy();
}

void PbrDrawable::Initialize(const std::string &name, Utils::Scene *scene, 
                             Render::GPUBuffer *lights, uint32_t numLight, 
                             Render::PixelBuffer *irradianceTex, Render::PixelBuffer *blurredEnvTex, Render::PixelBuffer *brdfLookupTex) {
    ASSERT_PRINT(scene && lights && irradianceTex);
    if (!scene || !lights || !irradianceTex || !blurredEnvTex || !brdfLookupTex) {
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

    mResourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(scene->mImages.size()) + 4);

    lights->CreateStructBufferSRV(mResourceHeap->Allocate(), numLight, sizeof(LightCB), false);
    irradianceTex->CreateSRV(mResourceHeap->Allocate(), false);
    blurredEnvTex->CreateSRV(mResourceHeap->Allocate(), false);
    brdfLookupTex->CreateSRV(mResourceHeap->Allocate(), false);

    if (scene->mImages.size() > 0) {
        mTextures.reserve(scene->mImages.size());
        for (auto image : scene->mImages) {
            Render::PixelBuffer *texture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), image->GetMipLevels(), image->GetDXGIFormat(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            texture->CreateSRV(mResourceHeap->Allocate());
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
    DeleteAndSetNull(mResourceHeap);
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

