#include "stdafx.h"
#include "PbrPass.h"
#include "PbrDrawable.h"
#include "pbr.vs.h"
#include "pbr.ps.h"

PbrPass::PbrPass(void)
: mRootSignature(nullptr)
, mGraphicsState(nullptr)
, mSamplerHeap(nullptr)
, mSampler(nullptr)
{
    Initialize();
}

PbrPass::~PbrPass(void) {
    Destroy();
}

void PbrPass::Initialize(void) {
    mRootSignature = new Render::RootSignature(Render::RootSignature::Graphics, 4, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignature->SetDescriptor(SettingsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 0);
    mRootSignature->SetDescriptor(CameraSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 1);
    mRootSignature->SetDescriptor(MaterialSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 2);
    mRootSignature->SetDescriptorTable(LightsSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
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

    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
    mSampler = new Render::Sampler();
    mSampler->Create(mSamplerHeap->Allocate());
}

void PbrPass::Destroy(void) {
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mGraphicsState);
    DeleteAndSetNull(mRootSignature);
}

void PbrPass::PreviousRender(void) {
    Render::gCommand->SetPipelineState(mGraphicsState);
    Render::gCommand->SetRootSignature(mRootSignature);
}

void PbrPass::Render(uint32_t currentFrame, PbrDrawable *drawable) {
    if (!drawable) {
        return;
    }

    Render::DescriptorHeap * resourceHeap = drawable->GetResourceHeap();
    Render::DescriptorHeap *heaps[] = { resourceHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    Render::gCommand->SetGraphicsRootConstantBufferView(0, drawable->GetSettingsCB(currentFrame));
    Render::gCommand->SetGraphicsRootConstantBufferView(1, drawable->GetCameraCB(currentFrame));
    Render::gCommand->SetGraphicsRootConstantBufferView(2, drawable->GetMaterialCB(currentFrame));
    Render::gCommand->SetGraphicsRootDescriptorTable(3, resourceHeap->GetHandle(0));
    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVerticesAndIndices(drawable->GetVertexBufferView(), drawable->GetIndexBufferView());

    for (auto &shape : drawable->GetShapes()) {
        //Render::gCommand->SetGraphicsRootDescriptorTable(1, mTextures[shape.diffuseTex]->GetSRVHandle());
        Render::gCommand->DrawIndexed(shape.indexCount, shape.indexOffset);
    }
}
