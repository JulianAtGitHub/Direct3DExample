#include "stdafx.h"
#include "SkyboxPass.h"
#include "skybox.vs.h"
#include "skybox.ps.h"

XMFLOAT3 SkyboxPass::msVertices[24] = {
    // Back face
    { -1.0f, -1.0f, -1.0f }, //0
    { -1.0f,  1.0f, -1.0f }, //1
    {  1.0f,  1.0f, -1.0f }, //2
    {  1.0f, -1.0f, -1.0f }, //3
    // Front face
    { -1.0f, -1.0f,  1.0f }, //4
    {  1.0f, -1.0f,  1.0f }, //5
    {  1.0f,  1.0f,  1.0f }, //6
    { -1.0f,  1.0f,  1.0f }, //7
    // Left face
    { -1.0f, -1.0f, -1.0f }, //8
    { -1.0f, -1.0f,  1.0f }, //9
    { -1.0f,  1.0f,  1.0f }, //10
    { -1.0f,  1.0f, -1.0f }, //11
    // Right face
    {  1.0f, -1.0f, -1.0f }, //12
    {  1.0f,  1.0f, -1.0f }, //13
    {  1.0f,  1.0f,  1.0f }, //14
    {  1.0f, -1.0f,  1.0f }, //15
    // Bottom face
    { -1.0f, -1.0f, -1.0f }, //16
    {  1.0f, -1.0f, -1.0f }, //17
    {  1.0f, -1.0f,  1.0f }, //18
    { -1.0f, -1.0f,  1.0f }, //19
    // Top face
    { -1.0f,  1.0f, -1.0f }, //20
    { -1.0f,  1.0f,  1.0f }, //21
    {  1.0f,  1.0f,  1.0f }, //22
    {  1.0f,  1.0f, -1.0f }, //23
};

uint16_t SkyboxPass::msIndices[36] = {
     0,  2,  3,    2,  0,  1,   // back
     4,  5,  6,    6,  7,  4,   // front
    10, 11,  8,    8,  9, 10,   // left
    14, 12, 13,   12, 14, 15,   // right
    16, 17, 18,   18, 19, 16,   // bottom
    20, 22, 23,   22, 20, 21,   // top
};

SkyboxPass::SkyboxPass(void)
: mRootSignature(nullptr)
, mGraphicsState(nullptr)
, mConstBuffer(nullptr)
, mVertexBuffer(nullptr)
, mIndexBuffer(nullptr)
, mTextureHeap(nullptr)
, mEnvTexture(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
{
    Initialize();
}

SkyboxPass::~SkyboxPass(void) {
    Destroy();
}

void SkyboxPass::Initialize(void) {
    mRootSignature = new Render::RootSignature(Render::RootSignature::Graphics, 3, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignature->SetDescriptor(0, D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    mRootSignature->SetDescriptorTable(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    mRootSignature->SetDescriptorTable(2, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mRootSignature->Create();

    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mGraphicsState = new Render::GraphicsState();
    mGraphicsState->GetInputLayout() = { inputElementDesc, _countof(inputElementDesc) };
    mGraphicsState->GetRasterizerState().FrontCounterClockwise = TRUE;
    mGraphicsState->GetRasterizerState().CullMode = D3D12_CULL_MODE_NONE;
    mGraphicsState->SetVertexShader(gscSkyboxVS, sizeof(gscSkyboxVS));
    mGraphicsState->SetPixelShader(gscSkyboxPS, sizeof(gscSkyboxPS));
    mGraphicsState->GetDepthStencilState().DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    mGraphicsState->Create(mRootSignature);

    mConstBuffer = new Render::ConstantBuffer(sizeof(TransformCB), 1);

    size_t verticesSize = sizeof(msVertices);
    size_t indicesSize = sizeof(msIndices);
    mVertexBuffer = new Render::GPUBuffer(verticesSize);
    mIndexBuffer = new Render::GPUBuffer(indicesSize);

    mTextureHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    Utils::Image *image = Utils::Image::CreateFromFile("..\\..\\Models\\WoodenDoor_Ref.hdr", false);
    mEnvTexture = new Render::PixelBuffer(image->GetPitch(), image->GetWidth(), image->GetHeight(), 1, image->GetDXGIFormat(), D3D12_RESOURCE_STATE_COMMON);

    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
    mSampler = new Render::Sampler();

    // upload data
    Render::gCommand->Begin();

    Render::gCommand->UploadBuffer(mVertexBuffer, 0, msVertices, verticesSize);
    Render::gCommand->UploadBuffer(mIndexBuffer, 0, msIndices, indicesSize);
    Render::gCommand->UploadTexture(mEnvTexture, image->GetPixels());

    Render::gCommand->End(true);

    mVertexBufferView = mVertexBuffer->FillVertexBufferView(0, static_cast<uint32_t>(verticesSize), sizeof(XMFLOAT3));
    mIndexBufferView = mIndexBuffer->FillIndexBufferView(0, static_cast<uint32_t>(indicesSize));

    mEnvTexture->CreateSRV(mTextureHeap->Allocate());
    mSampler->Create(mSamplerHeap->Allocate());

    delete image;
}

void SkyboxPass::Destroy(void) {
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mTextureHeap);
    DeleteAndSetNull(mEnvTexture);
    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mIndexBuffer);
    DeleteAndSetNull(mVertexBuffer);
    DeleteAndSetNull(mGraphicsState);
    DeleteAndSetNull(mRootSignature);
}

void SkyboxPass::Update(uint32_t currentFrame, const XMFLOAT4X4 &viewMatrix, const XMFLOAT4X4 projMatrix) {
    TransformCB cb;
    cb.ViewMat = viewMatrix;
    cb.ProjMat = projMatrix;
    mConstBuffer->CopyData(&cb, sizeof(TransformCB), 0, currentFrame);
}

void SkyboxPass::Render(uint32_t currentFrame) {
    Render::gCommand->SetPipelineState(mGraphicsState);
    Render::gCommand->SetRootSignature(mRootSignature);
    Render::DescriptorHeap *heaps[] = { mTextureHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    Render::gCommand->SetGraphicsRootConstantBufferView(0, mConstBuffer->GetGPUAddress(0, currentFrame));
    Render::gCommand->SetGraphicsRootDescriptorTable(1, mEnvTexture->GetSRVHandle());
    Render::gCommand->SetGraphicsRootDescriptorTable(2, mSampler->GetHandle());
    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVerticesAndIndices(mVertexBufferView, mIndexBufferView);
    Render::gCommand->DrawIndexed(INDEX_COUNT, 0);
}

