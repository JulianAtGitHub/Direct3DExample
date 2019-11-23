#include "stdafx.h"
#include "PbrExample.h"
#include "SkyboxPass.h"
#include "pbr.vs.h"
#include "pbr.ps.h"

std::string PbrExample::WindowTitle = "PBR Example";

PbrExample::PbrExample(void)
: Utils::AnExample()
, mWidth(0)
, mHeight(0)
, mCurrentFrame(0)
, mCamera(nullptr)
, mSkybox(nullptr)
, mRootSignature(nullptr)
, mGraphicsState(nullptr)
, mConstBuffer(nullptr)
, mVertexBuffer(nullptr)
, mIndexBuffer(nullptr)
{
    for (auto &fence : mFenceValues) {
        fence = 1;
    }
}

PbrExample::~PbrExample(void) {

}

void PbrExample::Init(HWND hwnd) {
    Utils::AnExample::Init(hwnd);
    WINDOWINFO windowInfo;
    GetWindowInfo(mHwnd, &windowInfo);
    mWidth = windowInfo.rcClient.right - windowInfo.rcClient.left;
    mHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;

    SetWindowText(mHwnd, WindowTitle.c_str());
    Render::Initialize(mHwnd);
    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();

    mCamera = new Utils::Camera(XM_PIDIV4, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.1f, 1000.0f, XMFLOAT4(0.0f, 0.0f, 3.0f, 0.0f));

    mSkybox = new SkyboxPass();

    mRootSignature = new Render::RootSignature(Render::RootSignature::Graphics, 1, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignature->SetDescriptor(0, D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    mRootSignature->Create();

    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mGraphicsState = new Render::GraphicsState();
    mGraphicsState->GetInputLayout() = { inputElementDesc, _countof(inputElementDesc) };
    mGraphicsState->GetRasterizerState().FrontCounterClockwise = TRUE;
    mGraphicsState->SetVertexShader(gscPbrVS, sizeof(gscPbrVS));
    mGraphicsState->SetPixelShader(gscPbrPS, sizeof(gscPbrPS));
    mGraphicsState->Create(mRootSignature);

    mConstBuffer = new Render::ConstantBuffer(sizeof(XMFLOAT4X4), 1);

    Utils::Scene *sphere = Utils::Model::LoadFromFile("..\\..\\Models\\Others\\sphere.obj");
    ASSERT_PRINT(sphere);

    const uint32_t vertexSize = static_cast<uint32_t>(sphere->mVertices.size() * sizeof(Utils::Scene::Vertex));
    const uint32_t indexSize = static_cast<uint32_t>(sphere->mIndices.size() * sizeof(uint32_t));
    mVertexBuffer = new Render::GPUBuffer(vertexSize);
    mIndexBuffer = new Render::GPUBuffer(indexSize);

    // upload data
    {
        Render::gCommand->Begin();

        Render::gCommand->UploadBuffer(mVertexBuffer, 0, sphere->mVertices.data(), vertexSize);
        Render::gCommand->UploadBuffer(mIndexBuffer, 0, sphere->mIndices.data(), indexSize);

        Render::gCommand->End(true);
    }

    mVertexBufferView = mVertexBuffer->FillVertexBufferView(0, vertexSize, sizeof(Utils::Scene::Vertex));
    mIndexBufferView = mIndexBuffer->FillIndexBufferView(0, indexSize, false);

    mShapes = sphere->mShapes;

    delete sphere;
}

void PbrExample::Destroy(void) {
    Render::gCommand->GetQueue()->WaitForIdle();

    DeleteAndSetNull(mCamera);
    DeleteAndSetNull(mSkybox);
    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mIndexBuffer);
    DeleteAndSetNull(mVertexBuffer);
    DeleteAndSetNull(mGraphicsState);
    DeleteAndSetNull(mRootSignature);

    Render::Terminate();
}

void PbrExample::Update(void) {
    XMFLOAT4X4 mvp;

    mCamera->UpdateMatrixs();
    XMMATRIX model = XMMatrixIdentity();
    XMMATRIX view = mCamera->GetViewMatrix();
    XMMATRIX proj = mCamera->GetProjectMatrix();
    XMStoreFloat4x4(&mvp, XMMatrixMultiply(XMMatrixMultiply(model, view), proj));

    XMFLOAT4X4 viewMat;
    XMFLOAT4X4 projMat;
    XMStoreFloat4x4(&viewMat, view);
    XMStoreFloat4x4(&projMat, proj);

    mSkybox->Update(mCurrentFrame, viewMat, projMat);
    mConstBuffer->CopyData(&mvp, sizeof(XMFLOAT4X4), 0, mCurrentFrame);
}

void PbrExample::Render(void) {
    Render::gCommand->GetQueue()->WaitForFence(mFenceValues[mCurrentFrame]);

    Render::gCommand->Begin(mGraphicsState);

    Render::gCommand->SetViewportAndScissor(0, 0, mWidth, mHeight);
    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);
    Render::gCommand->SetRenderTarget(Render::gRenderTarget[mCurrentFrame], Render::gDepthStencil);
    Render::gCommand->ClearColor(Render::gRenderTarget[mCurrentFrame]);
    Render::gCommand->ClearDepth(Render::gDepthStencil);

    Render::gCommand->SetRootSignature(mRootSignature);
    //Render::DescriptorHeap *heaps[] = { mShaderResourceHeap, mSamplerHeap };
    //Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    Render::gCommand->SetGraphicsRootConstantBufferView(0, mConstBuffer->GetGPUAddress(0, mCurrentFrame));
    //Render::gCommand->SetGraphicsRootDescriptorTable(2, mSampler->GetHandle());
    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVerticesAndIndices(mVertexBufferView, mIndexBufferView);

    //Render::gCommand->SetGraphicsRootDescriptorTable(1, mTextures[shape.diffuseTex]->GetSRVHandle());
    Render::gCommand->DrawIndexed(mShapes[0].indexCount, mShapes[0].indexOffset);

    mSkybox->Render(mCurrentFrame);

    Render::gCommand->TransitResource(Render::gRenderTarget[mCurrentFrame], D3D12_RESOURCE_STATE_PRESENT);

    mFenceValues[mCurrentFrame] = Render::gCommand->End();

    ASSERT_SUCCEEDED(Render::gSwapChain->Present(1, 0));

    mCurrentFrame = Render::gSwapChain->GetCurrentBackBufferIndex();
}
