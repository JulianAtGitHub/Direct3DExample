#include "stdafx.h"
#include "PbrPass.h"
#include "PbrDrawable.h"
#include "pbr.vs.h"
#include "pbr.ps.h"
#include "pbr.tex.ps.h"
#include "pbr.ibl.ps.h"
#include "pbr.tex.ibl.ps.h"
#include "pbr_d.ps.h"
#include "pbr_d.tex.ps.h"
#include "pbr_f.ps.h"
#include "pbr_f.tex.ps.h"
#include "pbr_g.ps.h"
#include "pbr_g.tex.ps.h"

PbrPass::PbrPass(void)
: mRootSignature(nullptr)
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
    mRootSignature->SetDescriptorTable(BlurredEnvTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    mRootSignature->SetDescriptorTable(BRDFLookupTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    mRootSignature->SetDescriptorTable(NormalTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
    mRootSignature->SetDescriptorTable(AlbdoTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
    mRootSignature->SetDescriptorTable(MetalnessTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
    mRootSignature->SetDescriptorTable(RoughnessTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7);
    mRootSignature->SetDescriptorTable(AOTexSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8);
    mRootSignature->SetDescriptorTable(SamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mRootSignature->Create();

    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    for (uint32_t i = 0; i < StateMax; ++i) {
        Render::GraphicsState *graphicsState = new Render::GraphicsState();
        graphicsState->GetInputLayout() = { inputElementDesc, _countof(inputElementDesc) };
        graphicsState->GetRasterizerState().FrontCounterClockwise = TRUE;
        graphicsState->SetVertexShader(gscPbrVS, sizeof(gscPbrVS));
        mGraphicsStates[i] = graphicsState;
    }
    mGraphicsStates[NoIBLNoTex]->SetPixelShader(gscPbrPS, sizeof(gscPbrPS));
    mGraphicsStates[NoIBLHasTex]->SetPixelShader(gscPbrTexPS, sizeof(gscPbrTexPS));
    mGraphicsStates[HasIBLNoTex]->SetPixelShader(gscPbrIblPS, sizeof(gscPbrIblPS));
    mGraphicsStates[HasIBLHasTex]->SetPixelShader(gscPbrTexIblPS, sizeof(gscPbrTexIblPS));
    mGraphicsStates[FactorFNoTex]->SetPixelShader(gscPbr_fPS, sizeof(gscPbr_fPS));
    mGraphicsStates[FactorFHasTex]->SetPixelShader(gscPbr_fTexPS, sizeof(gscPbr_fTexPS));
    mGraphicsStates[FactorDNoTex]->SetPixelShader(gscPbr_dPS, sizeof(gscPbr_dPS));
    mGraphicsStates[FactorDHasTex]->SetPixelShader(gscPbr_dTexPS, sizeof(gscPbr_dTexPS));
    mGraphicsStates[FactorGNoTex]->SetPixelShader(gscPbr_gPS, sizeof(gscPbr_gPS));
    mGraphicsStates[FactorGHasTex]->SetPixelShader(gscPbr_gTexPS, sizeof(gscPbr_gTexPS));
    for (uint32_t i = 0; i < StateMax; ++i) {
        mGraphicsStates[i]->Create(mRootSignature);
    }

    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
    mSampler = new Render::Sampler();
    mSampler->SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    mSampler->Create(mSamplerHeap->Allocate());
}

void PbrPass::Destroy(void) {
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mSamplerHeap);
    for (uint32_t i = 0; i < StateMax; ++i) {
        DeleteAndSetNull(mGraphicsStates[i]);
    }
    DeleteAndSetNull(mRootSignature);
}

void PbrPass::PreviousRender(State state) {
    Render::gCommand->SetPipelineState(mGraphicsStates[state]);
    Render::gCommand->SetRootSignature(mRootSignature);
}

void PbrPass::Render(uint32_t currentFrame, PbrDrawable *drawable, Render::DescriptorHeap *texHeap, uint32_t envIndex) {
    if (!drawable || !texHeap) {
        return;
    }

    Render::DescriptorHeap *heaps[] = { texHeap, mSamplerHeap };
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));

    Render::gCommand->SetGraphicsRootConstantBufferView(SettingsSlot, drawable->GetSettingsCB(currentFrame));
    Render::gCommand->SetGraphicsRootConstantBufferView(TransformSlot, drawable->GetTransformCB(currentFrame));
    Render::gCommand->SetGraphicsRootConstantBufferView(MaterialSlot, drawable->GetMaterialCB(currentFrame));
    Render::gCommand->SetGraphicsRootDescriptorTable(LightsSlot, texHeap->GetHandle(0));
    Render::gCommand->SetGraphicsRootDescriptorTable(BRDFLookupTexSlot, texHeap->GetHandle(1));
    Render::gCommand->SetGraphicsRootDescriptorTable(IrradianceTexSlot, texHeap->GetHandle(2 + 3 * envIndex + 1));
    Render::gCommand->SetGraphicsRootDescriptorTable(BlurredEnvTexSlot, texHeap->GetHandle(2 + 3 * envIndex + 2));
    Render::gCommand->SetGraphicsRootDescriptorTable(SamplerSlot, mSampler->GetHandle());

    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVerticesAndIndices(drawable->GetVertexBufferView(), drawable->GetIndexBufferView());

    uint32_t texOffset = drawable->GetMatTexOffset();
    for (auto &shape : drawable->GetShapes()) {
        Render::gCommand->SetGraphicsRootDescriptorTable(NormalTexSlot, texHeap->GetHandle(shape.normalTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(AlbdoTexSlot, texHeap->GetHandle(shape.albdoTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(MetalnessTexSlot, texHeap->GetHandle(shape.metalnessTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(RoughnessTexSlot, texHeap->GetHandle(shape.roughnessTex + texOffset));
        Render::gCommand->SetGraphicsRootDescriptorTable(AOTexSlot, texHeap->GetHandle(shape.albdoTex + texOffset));
        Render::gCommand->DrawIndexed(shape.indexCount, shape.indexOffset);
    }
}
