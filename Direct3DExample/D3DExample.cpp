#include "pch.h"
#include "D3DExample.h"
#include "Core/Model/Scene.h"
#include "Core/Model/Loader.h"
#include "Core/Render/CommandContext.h"
#include "Core/Render/DescriptorHeap.h"
#include "Core/Render/Resource/GPUBuffer.h"
#include "Core/Render/Resource/ConstantBuffer.h"
#include "Core/Render/Resource/PixelBuffer.h"
#include "Core/Render/Resource/RenderTargetBuffer.h"
#include "Core/Render/Sampler.h"
#include "Core/Render/PipelineState.h"
#include "Core/Render/RootSignature.h"

struct ConstBuffer {
    XMFLOAT4X4 mvp;
};

D3DExample::D3DExample(HWND hwnd)
: Example(hwnd)
, mBundleAllocator(nullptr)
, mRootSignature(nullptr)
, mGraphicsState(nullptr)
, mVertexIndexBuffer(nullptr)
, mConstBuffer(nullptr)
, mShaderResourceHeap(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
, mCurrentFrame(0)
, mScene(nullptr)
{
    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
        mBundles[i] = nullptr;
        mFenceValues[i] = 1;
    }
    mVertexBufferView = {};
    mIndexBufferView = {};

    WINDOWINFO windowInfo;
    GetWindowInfo(mHwnd, &windowInfo);
    mWidth = windowInfo.rcClient.right - windowInfo.rcClient.left;
    mHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;
}

D3DExample::~D3DExample(void) {
    if (mScene) {
        delete mScene;
    }
}

void D3DExample::Init(void) {
    LoadPipeline();
    LoadAssets();
}

void D3DExample::Update(void) {
    static float y = 0.0f;
    y += 0.01f;
    XMVECTOR cameraPos = ::XMVectorSet(0.0f, 1.0f, 5.0f, 0.0f);
    XMVECTOR cameraFocus = ::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR cameraUp = ::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    float cameraFOV = 60.0f * XM_PI / 180.0f;
    ConstBuffer constBuffer;

    //XMMATRIX model = ::XMMatrixIdentity();
    XMMATRIX model = ::XMMatrixRotationY(y);
    XMMATRIX view = ::XMMatrixLookAtRH(cameraPos, cameraFocus, cameraUp);
    XMMATRIX proj = ::XMMatrixPerspectiveFovRH(cameraFOV, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.1f, 100.0f);
    XMStoreFloat4x4(&constBuffer.mvp, ::XMMatrixTranspose(::XMMatrixMultiply(::XMMatrixMultiply(model, view), proj)));

    memcpy(mConstBuffer->GetMappedBuffer(0, mCurrentFrame), &constBuffer, sizeof(constBuffer));
}

void D3DExample::Render(void) {
    Render::gCommand->Begin(mGraphicsState->GetPipelineState());
    PopulateCommandList();
    mFenceValues[mCurrentFrame] = Render::gCommand->End();
    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));
    MoveToNextFrame();
}

void D3DExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    for (uint32_t i = 0; i < mTextures.Count(); ++i) {
        delete mTextures.At(i);
    };
    mTextures.Clear();
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mShaderResourceHeap);
    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mVertexIndexBuffer);
    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
        mBundles[i]->Release();
    }
    DeleteAndSetNull(mGraphicsState);
    DeleteAndSetNull(mRootSignature);
    mBundleAllocator->Release();

    Render::Terminate();
}

void D3DExample::MoveToNextFrame(void) {
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}

void D3DExample::LoadPipeline(void) {
    Render::Initialize(mHwnd);
    mShaderResourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);
    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    ASSERT_SUCCEEDED(Render::gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mBundleAllocator)));
}

void D3DExample::LoadAssets(void) {
    mScene = Model::Loader::LoadFromBinaryFile("Models\\Arwing\\arwing.bsx");
    assert(mScene);

    // root signature
    mRootSignature = new Render::RootSignature(3);
    mRootSignature->SetConstantBuffer(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
    mRootSignature->SetDescriptorTable(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_PIXEL);
    mRootSignature->SetDescriptorTable(2, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0);
    mRootSignature->Create();

    // input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mGraphicsState = new Render::GraphicsState();
    mGraphicsState->GetInputLayout() = { inputElementDesc, _countof(inputElementDesc) };
    mGraphicsState->GetRasterizerState().FrontCounterClockwise = TRUE;
    mGraphicsState->LoadVertexShader("color.vs.cso");
    mGraphicsState->LoadPixelShader("color.ps.cso");
    mGraphicsState->Create(mRootSignature->Get());

    mSampler = new Render::Sampler();
    mSampler->Create(mSamplerHeap->Allocate());

    Render::gCommand->Begin();

    // vertex and index
    {
        const uint32_t vertexSize = mScene->mVertices.Count() * sizeof(Model::Scene::Vertex);
        const uint32_t vertexAlignSize = AlignUp(vertexSize, 256);
        const uint32_t indexSize = mScene->mIndices.Count() * sizeof(uint32_t);
        const uint32_t indexAlignSize = AlignUp(indexSize, 256);
        mVertexIndexBuffer = new Render::GPUBuffer(vertexAlignSize + indexAlignSize);

        Render::gCommand->UploadBuffer(mVertexIndexBuffer, 0, mScene->mVertices.Data(), vertexSize);
        Render::gCommand->UploadBuffer(mVertexIndexBuffer, vertexAlignSize, mScene->mIndices.Data(), indexSize);

        mVertexBufferView = mVertexIndexBuffer->FillVertexBufferView(0, vertexSize, sizeof(Model::Scene::Vertex));
        mIndexBufferView = mVertexIndexBuffer->FillIndexBufferView(vertexAlignSize, indexSize, false);
    }

    mTextures.Reserve(mScene->mImages.Count());
    for (uint32_t i = 0; i < mScene->mImages.Count(); ++i) {
        Model::Scene::Image &image = mScene->mImages.At(i);
        Render::PixelBuffer *texture = new Render::PixelBuffer(image.width, image.width, image.height, DXGI_FORMAT_R8G8B8A8_UNORM);
        Render::gCommand->UploadTexture(texture, image.pixels);
        texture->CreateSRV(mShaderResourceHeap->Allocate());
        mTextures.PushBack(texture);
    }

    Render::gCommand->End(true);

    // const buffer
    mConstBuffer = new Render::ConstantBuffer(sizeof(ConstBuffer), 1);

    // bundle
    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
        ASSERT_SUCCEEDED(Render::gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mBundleAllocator, mGraphicsState->GetPipelineState(), IID_PPV_ARGS(mBundles + i)));

        // record command to bundle
        mBundles[i]->SetGraphicsRootSignature(mRootSignature->Get());
        ID3D12DescriptorHeap *heaps[] = { mShaderResourceHeap->Get(), mSamplerHeap->Get() };
        mBundles[i]->SetDescriptorHeaps(_countof(heaps), heaps);
        mBundles[i]->SetGraphicsRootConstantBufferView(0, mConstBuffer->GetGPUAddress(0, i));
        mBundles[i]->SetGraphicsRootDescriptorTable(2, mSampler->GetHandle().gpu);
        mBundles[i]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mBundles[i]->IASetVertexBuffers(0, 1, &mVertexBufferView);
        mBundles[i]->IASetIndexBuffer(&mIndexBufferView);
        for (uint32_t j = 0; j < mScene->mShapes.Count(); ++j) {
            const Model::Scene::Shape &shape = mScene->mShapes.At(j);
            mBundles[i]->SetGraphicsRootDescriptorTable(1, mTextures.At(shape.imageIndex)->GetHandle().gpu);
            mBundles[i]->DrawIndexedInstanced(shape.indexCount, 1, shape.fromIndex, 0, 0);
        }
        mBundles[i]->Close();
    }
}

void D3DExample::PopulateCommandList(void) {
    Render::gCommand->SetViewportAndScissor(0, 0, mWidth, mHeight);
    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);
    Render::gCommand->SetRenderTarget(Render::gRenderTarget[mCurrentFrame], Render::gDepthStencil);
    Render::gCommand->ClearColor(Render::gRenderTarget[mCurrentFrame]);
    Render::gCommand->ClearDepth(Render::gDepthStencil);
    Render::gCommand->SetRootSignature(mRootSignature);
    Render::DescriptorHeap *heaps[] = { mShaderResourceHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    Render::gCommand->ExecuteBundle(mBundles[mCurrentFrame]);
    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);
}

