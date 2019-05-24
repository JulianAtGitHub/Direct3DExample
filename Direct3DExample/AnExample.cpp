#include "pch.h"
#include "AnExample.h"
#include "Core/Model/Scene.h"
#include "Core/Model/Loader.h"
#include "Core/Render/RenderCore.h"
#include "Core/Render/CommandContext.h"
#include "Core/Render/DescriptorHeap.h"
#include "Core/Render/GPUBuffer.h"
#include "Core/Render/ConstantBuffer.h"
#include "Core/Render/PixelBuffer.h"

struct ConstBuffer {
    XMFLOAT4X4 mvp;
};

AnExample::AnExample(HWND hwnd)
:mHwnd(hwnd)
,mBundleAllocator(nullptr)
,mRootSignature(nullptr)
,mPipelineState(nullptr)
,mVertexIndexBuffer(nullptr)
,mConstBuffer(nullptr)
,mCurrentFrame(0)
,mScene(nullptr)
{
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
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
    Render::gCommand->Begin(mPipelineState);
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

    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mVertexIndexBuffer);
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        mBundles[i]->Release();
    }
    mPipelineState->Release();
    mRootSignature->Release();
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
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    result = Render::gDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
    if (FAILED(result)) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    CD3DX12_ROOT_PARAMETER1 rootParams[3];
    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParams[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[2].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob *signature = nullptr;
    ID3DBlob *error = nullptr;
    result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
    assert(SUCCEEDED(result));

    result = Render::gDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
    assert(SUCCEEDED(result));

    signature->Release();
    if (error) {
        error->Release();
    }

    // build pipeline state

    // shaders
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

    CD3DX12_SHADER_BYTECODE vsByteCode(vertexShader);
    CD3DX12_SHADER_BYTECODE psByteCode(pixelShader);

    // input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_INPUT_LAYOUT_DESC inputDesc = {};
    inputDesc.pInputElementDescs = inputElementDesc;
    inputDesc.NumElements = _countof(inputElementDesc);

    // resterizer
    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.FrontCounterClockwise = TRUE;
    
    // blend
    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);

    // depth stencil
    CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);

    // sample
    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;

    // pipeline 
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
    pipelineStateDesc.InputLayout = inputDesc;
    pipelineStateDesc.pRootSignature = mRootSignature;
    pipelineStateDesc.VS = vsByteCode;
    pipelineStateDesc.PS = psByteCode;
    pipelineStateDesc.RasterizerState = rasterizerDesc;
    pipelineStateDesc.BlendState = blendDesc;
    pipelineStateDesc.DepthStencilState = depthStencilDesc;
    pipelineStateDesc.SampleMask = UINT_MAX;
    pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateDesc.NumRenderTargets = 1;
    pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateDesc.SampleDesc = sampleDesc;

    result = Render::gDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&mPipelineState));
    assert(SUCCEEDED(result));

    vertexShader->Release();
    pixelShader->Release();

    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    Render::DescriptorHandle samplerHandle = Render::gSamplerHeap->Allocate();
    Render::gDevice->CreateSampler(&samplerDesc, samplerHandle.cpu);

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

    // texture
    {
        mTextures.Reserve(mScene->mImages.Count());
        for (uint32_t i = 0; i < mScene->mImages.Count(); ++i) {
            Model::Scene::Image &image = mScene->mImages.At(i);
            Render::PixelBuffer *texture = new Render::PixelBuffer(image.width, image.width, image.height, DXGI_FORMAT_R8G8B8A8_UNORM);
            Render::gCommand->UploadTexture(texture, image.pixels);
            texture->CreateSRView(Render::gShaderResourceHeap->Allocate());
            mTextures.PushBack(texture);
        }
    }

    Render::gCommand->End(true);

    // const buffer
    mConstBuffer = new Render::ConstantBuffer(sizeof(ConstBuffer), 1);

    // bundle
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        result = Render::gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mBundleAllocator, mPipelineState, IID_PPV_ARGS(&(mBundles[i])));
        assert(SUCCEEDED(result));

        // record command to bundle
        mBundles[i]->SetGraphicsRootSignature(mRootSignature);
        ID3D12DescriptorHeap *shaderResourceHeap = Render::gShaderResourceHeap->GetCurrentHeap();
        ID3D12DescriptorHeap *samplerHeap = Render::gSamplerHeap->GetCurrentHeap();
        ID3D12DescriptorHeap *heaps[] = { shaderResourceHeap, samplerHeap };
        mBundles[i]->SetDescriptorHeaps(_countof(heaps), heaps);
        mBundles[i]->SetGraphicsRootConstantBufferView(0, mConstBuffer->GetGPUAddress(0, i));
        mBundles[i]->SetGraphicsRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(samplerHeap->GetGPUDescriptorHandleForHeapStart()));
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

    ID3D12GraphicsCommandList *commandList = Render::gCommand->GetCommandList();
    commandList->SetGraphicsRootSignature(mRootSignature);

    ID3D12DescriptorHeap *shaderResourceHeap = Render::gShaderResourceHeap->GetCurrentHeap();
    ID3D12DescriptorHeap *samplerHeap = Render::gSamplerHeap->GetCurrentHeap();
    ID3D12DescriptorHeap *heaps[] = { shaderResourceHeap, samplerHeap };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    commandList->ExecuteBundle(mBundles[mCurrentFrame]);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);
}

uint8_t * AnExample::GenerateTextureData(void) {
    const uint32_t rowPitch = TEXTURE_WIDTH * TEXTURE_PIXEL_SIZE;
    const uint32_t cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
    const uint32_t cellHeight = TEXTURE_WIDTH >> 3;	// The height of a cell in the checkerboard texture.
    const uint32_t textureSize = rowPitch * TEXTURE_HEIGHT;

    uint8_t *pData = new uint8_t[textureSize];

    for (uint32_t n = 0; n < textureSize; n += TEXTURE_PIXEL_SIZE) {
        uint32_t x = n % rowPitch;
        uint32_t y = n / rowPitch;
        uint32_t i = x / cellPitch;
        uint32_t j = y / cellHeight;

        if (i % 2 == j % 2) {
            pData[n] = 0x00;		// R
            pData[n + 1] = 0x00;	// G
            pData[n + 2] = 0x00;	// B
            pData[n + 3] = 0xff;	// A
        } else {
            pData[n] = 0xff;		// R
            pData[n + 1] = 0xff;	// G
            pData[n + 2] = 0xff;	// B
            pData[n + 3] = 0xff;	// A
        }
    }

    return pData;
}
