#include "pch.h"
#include "AnExample.h"
#include "Core/Model/Scene.h"
#include "Core/Model/Loader.h"
#include "Core/Render/RenderCore.h"

struct ConstBuffer {
    XMFLOAT4X4 mvp;
};

AnExample::AnExample(HWND hwnd)
:mHwnd(hwnd)
,mBundleAllocator(nullptr)
,mCbvSrvHeap(nullptr)
,mSamplerHeap(nullptr)
,mRootSignature(nullptr)
,mPipelineState(nullptr)
,mCommandList(nullptr)
,mVertexBuffer(nullptr)
,mIndexBuffer(nullptr)
,mConstBuffer(nullptr)
,mFenceEvent(nullptr)
,mFence(nullptr)
,mCurrentFrame(0)
,mConstBufferSize(0)
,mScene(nullptr)
{
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        mCommandAllocators[i] = nullptr;
        mBundles[i] = nullptr;
        mDataBeginCB[i] = nullptr;
        mFenceValues[i] = 1;
    }
    mVertexBufferView = {};
    mIndexBufferView = {};

    WINDOWINFO windowInfo;
    GetWindowInfo(mHwnd, &windowInfo);
    mWidth = windowInfo.rcClient.right - windowInfo.rcClient.left;
    mHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;
    mAspectRatio = static_cast<float>(mWidth) / static_cast<float>(mHeight);
    mViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    mScissorRect = { 0, 0, static_cast<long>(mWidth), static_cast<long>(mHeight) };
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
    XMMATRIX proj = ::XMMatrixPerspectiveFovRH(cameraFOV, mAspectRatio, 0.1f, 100.0f);
    XMStoreFloat4x4(&constBuffer.mvp, ::XMMatrixTranspose(::XMMatrixMultiply(::XMMatrixMultiply(model, view), proj)));

    memcpy(mDataBeginCB[mCurrentFrame], &constBuffer, sizeof(constBuffer));
}

void AnExample::Render(void) {
    PopulateCommandList();
    
    ID3D12CommandList *commandLists[] = { mCommandList };
    Render::gCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    HRESULT result = Render::gSwapChain->Present(1, 0);

    MoveToNextFrame();
}

void AnExample::Destroy(void) {
    WaitForGPU();
    CloseHandle(mFenceEvent);

    mConstBuffer->Unmap(0, nullptr);

    mFence->Release();

    for (uint32_t i = 0; i < mTextures.Count(); ++i) {
        mTextures.At(i)->Release();
    };
    mConstBuffer->Release();
    mIndexBuffer->Release();
    mVertexBuffer->Release();
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        mBundles[i]->Release();
    }
    mCommandList->Release();
    mPipelineState->Release();
    mRootSignature->Release();
    mSamplerHeap->Release();
    mCbvSrvHeap->Release();
    mBundleAllocator->Release();
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        mCommandAllocators[i]->Release();
    }

    Render::Terminate();
}

void AnExample::WaitForGPU(void) {
    const uint64_t fenceValue = mFenceValues[mCurrentFrame];
    Render::gCommandQueue->Signal(mFence, fenceValue);
    mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
    WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
}

void AnExample::MoveToNextFrame(void) {
    const uint64_t fenceValue = mFenceValues[mCurrentFrame];
    Render::gCommandQueue->Signal(mFence, fenceValue);

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFenceValues[mCurrentFrame]) {
        mFence->SetEventOnCompletion(mFenceValues[mCurrentFrame], mFenceEvent);
        WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
    }

    mFenceValues[mCurrentFrame] = fenceValue + 1;
}

void AnExample::LoadPipeline(void) {
    HRESULT result = S_OK;

    Render::Initialize(mHwnd);

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    // command allocator
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        result = Render::gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(mCommandAllocators[i])));
        assert(SUCCEEDED(result));
    }

    result = Render::gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mBundleAllocator));
    assert(SUCCEEDED(result));
}

void AnExample::LoadAssets(void) {
    mScene = Model::Loader::LoadFromBinaryFile("Models\\Arwing\\arwing.bsx");
    assert(mScene);

    //Model::Loader::SaveToBinaryFile(mScene, "Models\\Arwing\\arwing.bsx");

    HRESULT result = S_OK;

    // const buffer view and shader resource view heap
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
    cbvSrvHeapDesc.NumDescriptors = mScene->mImages.Count();
    cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    result = Render::gDevice->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&mCbvSrvHeap));
    assert(SUCCEEDED(result));

    // sampler heap
    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = 1;
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    result = Render::gDevice->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&mSamplerHeap));
    assert(SUCCEEDED(result));

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
    //samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = 0.0f;
    //samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    Render::gDevice->CreateSampler(&samplerDesc, mSamplerHeap->GetCPUDescriptorHandleForHeapStart());

    // command list
    result = Render::gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[mCurrentFrame], mPipelineState, IID_PPV_ARGS(&mCommandList));
    assert(SUCCEEDED(result));

    // vertex buffer
    ID3D12Resource *vertUploadHeap = nullptr;
    {
        const uint32_t bufferSize = mScene->mVertices.Count() * sizeof(Model::Scene::Vertex);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        result = Render::gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mVertexBuffer));
        assert(SUCCEEDED(result));

        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        result = Render::gDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertUploadHeap));
        assert(SUCCEEDED(result));

        D3D12_SUBRESOURCE_DATA subResData = {};
        subResData.pData = mScene->mVertices.Data();
        subResData.RowPitch = bufferSize;
        subResData.SlicePitch = subResData.RowPitch;

        UpdateSubresources(mCommandList, mVertexBuffer, vertUploadHeap, 0, 0, 1, &subResData);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        mCommandList->ResourceBarrier(1, &barrier);

        // vertex buffer view
        mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
        mVertexBufferView.StrideInBytes = sizeof(Model::Scene::Vertex);
        mVertexBufferView.SizeInBytes = bufferSize;
    }

    // index buffer
    ID3D12Resource *idxUploadHeap = nullptr;
    {
        const uint32_t bufferSize = mScene->mIndices.Count() * sizeof(uint32_t);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        result = Render::gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mIndexBuffer));
        assert(SUCCEEDED(result));

        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        result = Render::gDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxUploadHeap));
        assert(SUCCEEDED(result));

        D3D12_SUBRESOURCE_DATA subResData = {};
        subResData.pData = mScene->mIndices.Data();
        subResData.RowPitch = bufferSize;
        subResData.SlicePitch = subResData.RowPitch;

        UpdateSubresources(mCommandList, mIndexBuffer, idxUploadHeap, 0, 0, 1, &subResData);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        mCommandList->ResourceBarrier(1, &barrier);

        mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
        mIndexBufferView.SizeInBytes = bufferSize;
        mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }

    // const buffer
    {
        mConstBufferSize = (sizeof(ConstBuffer) + 255) & ~255; // CB size is required to be 256-byte aligned.
        CD3DX12_HEAP_PROPERTIES cbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC cbResDesc = CD3DX12_RESOURCE_DESC::Buffer(mConstBufferSize * FRAME_COUNT);
        result = Render::gDevice->CreateCommittedResource(&cbHeapProps, D3D12_HEAP_FLAG_NONE, &cbResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mConstBuffer));
        assert(SUCCEEDED(result));

        uint8_t *pDataBeginCB = nullptr;
        CD3DX12_RANGE readRangeCB(0, 0);
        result = mConstBuffer->Map(0, &readRangeCB, (void**)&pDataBeginCB);
        assert(SUCCEEDED(result));
        for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
            mDataBeginCB[i] = pDataBeginCB + mConstBufferSize * i;
        }
    }

    // texture
    mTextures.Reserve(mScene->mImages.Count());
    CList<ID3D12Resource *> textureUploadHeaps(mScene->mImages.Count());
    UINT srvDescriptorSize = Render::gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandler(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
        for (uint32_t i = 0; i < mScene->mImages.Count(); ++i) {
            Model::Scene::Image &image = mScene->mImages.At(i);
            ID3D12Resource *aTexture = nullptr;
            ID3D12Resource *textureUploadHeap = nullptr;
            uint8_t *rawData = image.pixels;

            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
            CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, image.width, image.height);
            result = Render::gDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&aTexture));
            assert(SUCCEEDED(result));

            CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC texUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(GetRequiredIntermediateSize(aTexture, 0, 1));
            result = Render::gDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &texUploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap));
            assert(SUCCEEDED(result));

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = rawData;
            textureData.RowPitch = image.width * image.channels;
            textureData.SlicePitch = textureData.RowPitch * image.height;

            UpdateSubresources(mCommandList, aTexture, textureUploadHeap, 0, 0, 1, &textureData);
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(aTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            mCommandList->ResourceBarrier(1, &barrier);

            // texture resource view
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            Render::gDevice->CreateShaderResourceView(aTexture, &srvDesc, srvHandler);
            srvHandler.Offset(1, srvDescriptorSize);

            mTextures.PushBack(aTexture);
            textureUploadHeaps.PushBack(textureUploadHeap);
        }
    }

    // fence
    result = Render::gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    assert(SUCCEEDED(result));

    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFence == nullptr) {
        result = HRESULT_FROM_WIN32(GetLastError());
        assert(SUCCEEDED(result));
    }

    mCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { mCommandList };
    Render::gCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    WaitForGPU();

    vertUploadHeap->Release();
    idxUploadHeap->Release();

    for (uint32_t i = 0; i < textureUploadHeaps.Count(); ++i) {
        textureUploadHeaps.At(i)->Release();
    }
    textureUploadHeaps.Clear();

    // bundle
    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        result = Render::gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mBundleAllocator, mPipelineState, IID_PPV_ARGS(&(mBundles[i])));
        assert(SUCCEEDED(result));

        // record command to bundle
        mBundles[i]->SetGraphicsRootSignature(mRootSignature);
        ID3D12DescriptorHeap *heaps[] = { mCbvSrvHeap, mSamplerHeap };
        mBundles[i]->SetDescriptorHeaps(_countof(heaps), heaps);
        mBundles[i]->SetGraphicsRootConstantBufferView(0, mConstBuffer->GetGPUVirtualAddress() + i * mConstBufferSize);
        //mBundles[i]->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart()));
        mBundles[i]->SetGraphicsRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(mSamplerHeap->GetGPUDescriptorHandleForHeapStart()));
        mBundles[i]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mBundles[i]->IASetVertexBuffers(0, 1, &mVertexBufferView);
        mBundles[i]->IASetIndexBuffer(&mIndexBufferView);
        for (uint32_t j = 0; j < mScene->mShapes.Count(); ++j) {
            const Model::Scene::Shape &shape = mScene->mShapes.At(j);
            CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandler(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
            srvHandler.Offset(shape.imageIndex, srvDescriptorSize);
            mBundles[i]->SetGraphicsRootDescriptorTable(1, srvHandler);
            mBundles[i]->DrawIndexedInstanced(shape.indexCount, 1, shape.fromIndex, 0, 0);
        }
        mBundles[i]->Close();
    }
}

void AnExample::PopulateCommandList(void) {
    HRESULT result = S_OK;

    // reset commands
    result = mCommandAllocators[mCurrentFrame]->Reset();
    assert(SUCCEEDED(result));

    result = mCommandList->Reset(mCommandAllocators[mCurrentFrame], mPipelineState);
    assert(SUCCEEDED(result));

    mCommandList->SetGraphicsRootSignature(mRootSignature);

    ID3D12DescriptorHeap *heaps[] = { mCbvSrvHeap, mSamplerHeap };
    mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    ID3D12Resource *renderTarget =  Render::GetRenderTarget(mCurrentFrame);
    D3D12_RESOURCE_BARRIER barrierBefore = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrierBefore);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Render::GetRenderTargetViewHandle(mCurrentFrame);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Render::GetDepthStencilViewHandle();
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // record commands
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    mCommandList->ExecuteBundle(mBundles[mCurrentFrame]);

    D3D12_RESOURCE_BARRIER barrierAfter = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrierAfter);

    mCommandList->Close();
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
