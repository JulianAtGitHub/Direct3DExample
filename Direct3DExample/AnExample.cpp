#include "pch.h"
#include "AnExample.h"
#include "Core/Model/Scene.h"
#include "Core/Model/Loader.h"
#include "Core/Render/CommandContext.h"
#include "Core/Render/DescriptorHeap.h"
#include "Core/Render/GPUBuffer.h"
#include "Core/Render/ConstantBuffer.h"
#include "Core/Render/PixelBuffer.h"
#include "Core/Render/Sampler.h"
#include "Core/Render/PipelineState.h"
#include "Core/Render/RootSignature.h"

struct ConstBuffer {
    XMFLOAT4X4 mvp;
};

AnExample::AnExample(HWND hwnd)
:mHwnd(hwnd)
,mBundleAllocator(nullptr)
,mRootSignature(nullptr)
,mGraphicsState(nullptr)
,mVertexIndexBuffer(nullptr)
,mConstBuffer(nullptr)
,mShaderResourceHeap(nullptr)
,mSamplerHeap(nullptr)
,mSampler(nullptr)
,mCurrentFrame(0)
,mScene(nullptr)
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

AnExample::~AnExample(void) {
    if (mScene) {
        delete mScene;
    }
}

void AnExample::Init(void) {
    LoadPipeline();
    LoadAssets();
}

void AnExample::Update(void) {
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

void AnExample::Render(void) {
    Render::gCommand->Begin(mGraphicsState->GetPipelineState());
    PopulateCommandList();
    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    HRESULT result = Render::gSwapChain->Present(1, 0);

    MoveToNextFrame();
}

void AnExample::Destroy(void) {
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

void AnExample::MoveToNextFrame(void) {
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}

void AnExample::LoadPipeline(void) {
    HRESULT result = S_OK;

    Render::Initialize(mHwnd);
    mShaderResourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);
    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    result = Render::gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mBundleAllocator));
    assert(SUCCEEDED(result));
}

void AnExample::LoadAssets(void) {
    mScene = Model::Loader::LoadFromBinaryFile("Models\\Arwing\\arwing.bsx");
    assert(mScene);

    //Model::Loader::SaveToBinaryFile(mScene, "Models\\Arwing\\arwing.bsx");

    HRESULT result = S_OK;

    // root signature
    mRootSignature = new Render::RootSignature(3);
    mRootSignature->SetConstantBuffer(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
    mRootSignature->SetDescriptorTable(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_PIXEL);
    mRootSignature->SetDescriptorTable(2, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0);
    mRootSignature->Create();

    // shaders
    ID3DBlob *error = nullptr;
    ID3DBlob *vertexShader = nullptr;
    ID3DBlob *pixelShader = nullptr;
    uint32_t compileFlags = 0;
#if defined(_DEBUG)
    compileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
#endif

    result = D3DCompileFromFile(L"Shaders\\color3d.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error);
    if (error) {
        OutputDebugStringA( (char*)error->GetBufferPointer() );
        error->Release();
    }
    assert(SUCCEEDED(result));

    result = D3DCompileFromFile(L"Shaders\\color3d.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error);
    if (error) {
        OutputDebugStringA( (char*)error->GetBufferPointer() );
        error->Release();
    }
    assert(SUCCEEDED(result));

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
    mGraphicsState->GetVertexShader() = CD3DX12_SHADER_BYTECODE(vertexShader);
    mGraphicsState->GetPixelShader() = CD3DX12_SHADER_BYTECODE(pixelShader);
    mGraphicsState->Create(mRootSignature->Get());

    vertexShader->Release();
    pixelShader->Release();

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
        texture->CreateSRView(mShaderResourceHeap->Allocate());
        mTextures.PushBack(texture);
    }

    Render::gCommand->End(true);

    // const buffer
    mConstBuffer = new Render::ConstantBuffer(sizeof(ConstBuffer), 1);

    // bundle
    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
        result = Render::gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mBundleAllocator, mGraphicsState->GetPipelineState(), IID_PPV_ARGS(&(mBundles[i])));
        assert(SUCCEEDED(result));

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

void AnExample::PopulateCommandList(void) {
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

