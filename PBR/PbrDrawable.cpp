#include "stdafx.h"
#include "PbrDrawable.h"


PbrDrawable::PbrDrawable(void)
: mResourceHeap(nullptr)
, mSettingsCB(nullptr)
, mTransformCB(nullptr)
, mMatValuesCB(nullptr)
, mMaterialBuffer(nullptr)
, mVertexBuffer(nullptr)
, mIndexBuffer(nullptr)
, mMatTexsOffset(ENV_HEAP_INDEX)
{

}

PbrDrawable::~PbrDrawable(void) {
    Destroy();
}

void PbrDrawable::Initialize(const std::string &name, Utils::Scene *scene, Render::GPUBuffer *lightBuffer, uint32_t lightCount, Render::PixelBuffer **envTexs, uint32_t envTexCount) {
    ASSERT_PRINT(scene && lightBuffer && envTexs);
    if (!scene || !lightBuffer || !envTexs) {
        return;
    }

    mName = name;

    mSettingsCB = new Render::ConstantBuffer(sizeof(SettingsCB), 1);
    mTransformCB = new Render::ConstantBuffer(sizeof(TransformCB), 1);
    mMatValuesCB = new Render::ConstantBuffer(sizeof(MatValuesCB), static_cast<uint32_t>(scene->mShapes.size()));

    size_t verticesSize = scene->mVertices.size() * sizeof(Utils::Scene::Vertex);
    size_t indicesSize = scene->mIndices.size() * sizeof(uint32_t);

    mVertexBuffer = new Render::GPUBuffer(verticesSize);
    mIndexBuffer = new Render::GPUBuffer(indicesSize);

    mVertexBufferView = mVertexBuffer->FillVertexBufferView(0, static_cast<uint32_t>(verticesSize), sizeof(Utils::Scene::Vertex));
    mIndexBufferView = mIndexBuffer->FillIndexBufferView(0, static_cast<uint32_t>(indicesSize), false);

    size_t matCount = scene->mMaterials.size();
    mMaterialBuffer = new Render::GPUBuffer(sizeof(MaterialCB) * matCount);
    MaterialCB *materials = new MaterialCB[matCount];
    for (size_t i = 0; i < matCount; ++i) {
        auto &mat = scene->mMaterials[i];
        MaterialCB &matCB = materials[i];
        matCB.normalScale = mat.normalScale;
        matCB.normalTexture = mat.normalTexture;
        matCB.occlusionStrength = mat.occlusionStrength;
        matCB.occlusionTexture = mat.occlusionTexture;
        matCB.emissiveFactor = mat.emissiveFactor;
        matCB.emissiveTexture = mat.emissiveTexture;
        matCB.baseFactor = mat.baseFactor;
        matCB.baseTexture = mat.baseTexture;
        matCB.metallicFactor = mat.metallicFactor;
        matCB.metallicTexture = mat.metallicTexture;
        matCB.roughnessFactor = mat.roughnessFactor;
        matCB.roughnessTexture = mat.roughnessTexture;
    }
    Render::gCommand->Begin();
    Render::gCommand->UploadBuffer(mMaterialBuffer, 0, materials, mMaterialBuffer->GetBufferSize());
    Render::gCommand->End(true);
    delete [] materials;

    mResourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(scene->mImages.size() + envTexCount + 2)); // light buffer + material buffer

    lightBuffer->CreateStructBufferSRV(mResourceHeap->Allocate(), lightCount, sizeof(LightCB), false);
    mMaterialBuffer->CreateStructBufferSRV(mResourceHeap->Allocate(), static_cast<uint32_t>(matCount), sizeof(MaterialCB));

    for (uint32_t i = 0; i < envTexCount; ++i) {
        envTexs[i]->CreateSRV(mResourceHeap->Allocate(), false);
    }

    mMatTexsOffset = (envTexCount + 2);
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

    // generate mipmaps
    for (auto texture : mTextures) {
        Utils::gMipsGener->Dispatch(texture);
    }

    mShapes = scene->mShapes;

    mTransform = XMLoadFloat4x4(&(scene->mTransform));
}

void PbrDrawable::Destroy(void) {
    DeleteAndSetNull(mResourceHeap);
    DeleteAndSetNull(mSettingsCB);
    DeleteAndSetNull(mTransformCB);
    DeleteAndSetNull(mMatValuesCB);
    DeleteAndSetNull(mMaterialBuffer);
    for (auto texture : mTextures) {
        delete texture;
    }
    mTextures.clear();
    DeleteAndSetNull(mIndexBuffer);
    DeleteAndSetNull(mVertexBuffer);
}

void PbrDrawable::Update(uint32_t currentFrame, Utils::Camera &camera, const SettingsCB &settings, const MatValuesCB &matValues) {
    XMMATRIX view = camera.GetViewMatrix();
    XMMATRIX proj = camera.GetProjectMatrix();

    TransformCB transform;
    XMStoreFloat4x4(&(transform.model), mTransform);
    XMStoreFloat4x4(&(transform.mvp), XMMatrixMultiply(XMMatrixMultiply(mTransform, view), proj));
    XMStoreFloat4(&(transform.cameraPos), camera.GetPosition());

    mSettingsCB->CopyData(&settings, sizeof(SettingsCB), 0, currentFrame);
    mTransformCB->CopyData(&transform, sizeof(TransformCB), 0, currentFrame);
    for (uint32_t i = 0; i < mShapes.size(); ++i) {
        MatValuesCB matValue = matValues;
        matValue.matIndex = mShapes[i].materialIndex;
        mMatValuesCB->CopyData(&matValue, sizeof(MatValuesCB), i, currentFrame);
    }
}

