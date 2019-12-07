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
    mRootSignature = new Render::RootSignature(Render::RootSignature::Graphics, SlotCount, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignature->SetDescriptor(SettingsSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 0);
    mRootSignature->SetDescriptor(TransformSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 1);
    mRootSignature->SetDescriptor(MaterialSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 2);
    mRootSignature->SetDescriptorTable(LightsSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    mRootSignature->SetDescriptorTable(IrradianceTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    mRootSignature->SetDescriptorTable(NormalTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    mRootSignature->SetDescriptorTable(AlbdoTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    mRootSignature->SetDescriptorTable(MetalnessTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
    mRootSignature->SetDescriptorTable(RoughnessTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
    mRootSignature->SetDescriptorTable(AOTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
    mRootSignature->SetDescriptorTable(SamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
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
    Render::gCommand->SetGraphicsRootConstantBufferView(SettingsSlot, drawable->GetSettingsCB(currentFrame));
    Render::gCommand->SetGraphicsRootConstantBufferView(TransformSlot, drawable->GetTransformCB(currentFrame));
    Render::gCommand->SetGraphicsRootConstantBufferView(MaterialSlot, drawable->GetMaterialCB(currentFrame));
    Render::gCommand->SetGraphicsRootDescriptorTable(LightsSlot, resourceHeap->GetHandle(0));
    Render::gCommand->SetGraphicsRootDescriptorTable(IrradianceTexSlot, resourceHeap->GetHandle(1));
    Render::gCommand->SetGraphicsRootDescriptorTable(SamplerSlot, mSampler->GetHandle());
    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVerticesAndIndices(drawable->GetVertexBufferView(), drawable->GetIndexBufferView());
    uint32_t texOffset = 2;
    for (auto &shape : drawable->GetShapes()) {
        Render::gCommand->SetGraphicsRootDescriptorTable(NormalTexSlot, resourceHeap->GetHandle(shape.normalTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(AlbdoTexSlot, resourceHeap->GetHandle(shape.albdoTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(MetalnessTexSlot, resourceHeap->GetHandle(shape.metalnessTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(RoughnessTexSlot, resourceHeap->GetHandle(shape.roughnessTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(AOTexSlot, resourceHeap->GetHandle(shape.albdoTex + texOffset));
        Render::gCommand->DrawIndexed(shape.indexCount, shape.indexOffset);
    }
}
