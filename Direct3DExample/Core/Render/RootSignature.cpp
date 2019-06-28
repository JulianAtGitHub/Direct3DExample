#include "pch.h"
#include "RootSignature.h"
#include "RenderCore.h"

namespace Render {

RootSignature::RootSignature(uint32_t paramCount)
: mParameters(mParameters)
, mRootSignature(nullptr)
{
    ASSERT_PRINT(paramCount > 0);

    mParameters.Resize(paramCount);
    for (uint32_t i = 0; i < paramCount; ++i) {
        mParameters.At(i).ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
    }
}

RootSignature::~RootSignature(void) {
    for (uint32_t i = 0; i < mParameters.Count(); ++i) {
        CleanupDescriptorRanges(mParameters.At(i));
    }
    ReleaseAndSetNull(mRootSignature);
}

void RootSignature::Create(void) {
    ReleaseAndSetNull(mRootSignature);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(mParameters.Count(), mParameters.Data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob *signature = nullptr;
    ID3DBlob *error = nullptr;
    D3D_ROOT_SIGNATURE_VERSION version = gRootSignatureSupport_Version_1_1 ? D3D_ROOT_SIGNATURE_VERSION_1_1 : D3D_ROOT_SIGNATURE_VERSION_1_0;

    ASSERT_SUCCEEDED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, version, &signature, &error));
    ASSERT_SUCCEEDED(gDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

    ReleaseAndSetNull(signature);
    ReleaseAndSetNull(error);
}

}