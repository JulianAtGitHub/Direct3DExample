#include "pch.h"
#include "D3DExample.h"

struct ConstBuffer {
    XMFLOAT4X4 mvp;
};

D3DExample::D3DExample(HWND hwnd)
: Example(hwnd)
, mRootSignature(nullptr)
, mGraphicsState(nullptr)
, mVertexIndexBuffer(nullptr)
, mConstBuffer(nullptr)
, mShaderResourceHeap(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
, mCurrentFrame(0)
{
    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
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

}

void D3DExample::Init(void) {
    LoadPipeline();
    LoadAssets();
}

void D3DExample::Update(void) {
    static float y = 0.0f;
    y += 0.01f;
    XMVECTOR cameraPos = { -2.706775665283203f, 0.85294109582901f, -3.112438678741455f, 1.0f };//::XMVectorSet(0.0f, 1.0f, 5.0f, 0.0f);
    XMVECTOR cameraFocus = {-2.347264528274536f, 0.7383297681808472f, -2.1863629817962648f, 1.0f};//::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR cameraUp = { 0.038521841168403628f, 0.9933950304985046f, 0.1079813688993454f, 0.0f }; //::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    float cameraFOV = XM_PIDIV4;
    ConstBuffer constBuffer;

    XMMATRIX model = ::XMMatrixIdentity();
    //XMMATRIX model = ::XMMatrixRotationY(y);
    XMMATRIX view = ::XMMatrixLookAtRH(cameraPos, cameraFocus, cameraUp);
    XMMATRIX proj = ::XMMatrixPerspectiveFovRH(cameraFOV, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.1f, 100.0f);
    XMStoreFloat4x4(&constBuffer.mvp, ::XMMatrixTranspose(::XMMatrixMultiply(::XMMatrixMultiply(model, view), proj)));

    mConstBuffer->CopyData(&constBuffer, sizeof(constBuffer), 0, mCurrentFrame);
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

    DeleteAndSetNull(mScene);

    for (auto texture : mTextures) { delete texture; }
    mTextures.clear();
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mShaderResourceHeap);
    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mVertexIndexBuffer);
    DeleteAndSetNull(mGraphicsState);
    DeleteAndSetNull(mRootSignature);

    Render::Terminate();
}

void D3DExample::MoveToNextFrame(void) {
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);
}

void D3DExample::LoadPipeline(void) {
    Render::Initialize(mHwnd);
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
}

void D3DExample::LoadAssets(void) {
    mScene = Utils::Model::LoadFromMMB("Models\\pink_room.mmb");
    assert(mScene);

    //mScene = Utils::Model::LoadFromFile("Models\\pink_room\\pink_room.fbx");
    //assert(mScene);
    //Utils::Model::SaveToMMB(mScene, "Models\\pink_room\\pink_room.mmb");

    mShaderResourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(mScene->mImages.size()));
    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    // root signature
    mRootSignature = new Render::RootSignature(Render::RootSignature::Graphics, 3, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignature->SetDescriptor(0, D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    mRootSignature->SetDescriptorTable(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    mRootSignature->SetDescriptorTable(2, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
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
        const uint32_t vertexSize = static_cast<uint32_t>(mScene->mVertices.size() * sizeof(Utils::Scene::Vertex));
        const uint32_t vertexAlignSize = AlignUp(vertexSize, 256);
        const uint32_t indexSize = static_cast<uint32_t>(mScene->mIndices.size() * sizeof(uint32_t));
        const uint32_t indexAlignSize = AlignUp(indexSize, 256);
        mVertexIndexBuffer = new Render::GPUBuffer(vertexAlignSize + indexAlignSize);

        Render::gCommand->UploadBuffer(mVertexIndexBuffer, 0, mScene->mVertices.data(), vertexSize);
        Render::gCommand->UploadBuffer(mVertexIndexBuffer, vertexAlignSize, mScene->mIndices.data(), indexSize);

        mVertexBufferView = mVertexIndexBuffer->FillVertexBufferView(0, vertexSize, sizeof(Utils::Scene::Vertex));
        mIndexBufferView = mVertexIndexBuffer->FillIndexBufferView(vertexAlignSize, indexSize, false);
    }

    mTextures.reserve(mScene->mImages.size());
    for (auto image : mScene->mImages) {
        Render::PixelBuffer *texture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), image->GetDXGIFormat());
        Render::gCommand->UploadTexture(texture, image->GetPixels());
        texture->CreateSRV(mShaderResourceHeap->Allocate());
        mTextures.push_back(texture);
    }

    Render::gCommand->End(true);

    // const buffer
    mConstBuffer = new Render::ConstantBuffer(sizeof(ConstBuffer), 1);
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
    Render::gCommand->SetGraphicsRootConstantBufferView(0, mConstBuffer->GetGPUAddress(0, mCurrentFrame));
    Render::gCommand->SetGraphicsRootDescriptorTable(2, mSampler->GetHandle());
    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVerticesAndIndices(mVertexBufferView, mIndexBufferView);
    for (uint32_t j = 0; j < mScene->mShapes.size(); ++j) {
        const Utils::Scene::Shape &shape = mScene->mShapes[j];
        Render::gCommand->SetGraphicsRootDescriptorTable(1, mTextures[shape.diffuseTex]->GetHandle());
        Render::gCommand->DrawIndexed(shape.indexCount, shape.indexOffset);
    }

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);
}

