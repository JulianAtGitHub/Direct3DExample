#pragma once

namespace Render {

class RootSignature {
public:
    RootSignature(uint32_t paramCount);
    ~RootSignature(void);

    INLINE ID3D12RootSignature * Get(void) const { return mRootSignature; }

    void Create(void);

    void SetConstantBuffer(
        uint32_t index, 
        uint32_t shaderRegister,
        D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    void SetDescriptorTable(
        uint32_t index,
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
        uint32_t shaderRegister,
        D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

private:
    void CleanupDescriptorRanges(D3D12_ROOT_PARAMETER1 &param);

    CList<D3D12_ROOT_PARAMETER1>            mParameters;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC     mDescription;
    ID3D12RootSignature                    *mRootSignature;
};

INLINE void RootSignature::CleanupDescriptorRanges(D3D12_ROOT_PARAMETER1 &param) {
    if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE 
        && param.DescriptorTable.pDescriptorRanges) {
        delete param.DescriptorTable.pDescriptorRanges;
        param.DescriptorTable.pDescriptorRanges = nullptr;
    }
}

INLINE void RootSignature::SetConstantBuffer(uint32_t index, uint32_t shaderRegister, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility) {
    if (index >= mParameters.Count()) {
        return;
    }

    auto & param = mParameters.At(index);
    CleanupDescriptorRanges(param);

    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = 0;
    param.Descriptor.Flags = flags;
}

INLINE void RootSignature::SetDescriptorTable(uint32_t index, D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t shaderRegister, D3D12_DESCRIPTOR_RANGE_FLAGS flags, D3D12_SHADER_VISIBILITY visibility) {
    if (index >= mParameters.Count()) {
        return;
    }

    auto & param = mParameters.At(index);
    CleanupDescriptorRanges(param);

    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = visibility;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = new CD3DX12_DESCRIPTOR_RANGE1(rangeType, 1, shaderRegister, 0, flags);
}

}