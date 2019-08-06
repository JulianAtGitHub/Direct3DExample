#include "stdafx.h"
#include "RootSignature.h"
#include "RenderCore.h"

namespace Render {

RootSignature::RootSignature(Type type, uint32_t paramCount, D3D12_ROOT_SIGNATURE_FLAGS flags)
: mType(type)
, mParameters(paramCount)
, mFlags(flags)
, mRootSignature(nullptr)
{
    for (auto &parameter : mParameters) {
        parameter.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
    }
}

RootSignature::~RootSignature(void) {
    for (auto &parameter : mParameters) {
        CleanupDescriptorRanges(parameter);
    }
    ReleaseAndSetNull(mRootSignature);
}

void RootSignature::Create(void) {
    ReleaseAndSetNull(mRootSignature);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(D3D12_DEFAULT);
    if (mParameters.size()) {
        rootSignatureDesc.Init(static_cast<uint32_t>(mParameters.size()), mParameters.data());
    }
    rootSignatureDesc.Flags = mFlags;

    WRL::ComPtr<ID3DBlob> blob;
    WRL::ComPtr<ID3DBlob> error;
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
    ASSERT_SUCCEEDED(gDevice->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

}