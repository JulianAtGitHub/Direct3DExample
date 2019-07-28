#pragma once

namespace Render {

class RootSignature {
public:
    RootSignature(uint32_t paramCount, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ~RootSignature(void);

    INLINE ID3D12RootSignature * Get(void) const { return mRootSignature; }

    void Create(void);

    void SetConstants(
        uint32_t index, 
        uint32_t num32BitValues,
        uint32_t shaderRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    void SetDescriptor(
        uint32_t index, 
        D3D12_ROOT_PARAMETER_TYPE type,
        uint32_t shaderRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    void SetDescriptorTable(
        uint32_t index,
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType, 
        uint32_t descCount,
        uint32_t baseRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

private:
    void CleanupDescriptorRanges(D3D12_ROOT_PARAMETER &param);

    CList<D3D12_ROOT_PARAMETER>     mParameters;
    D3D12_ROOT_SIGNATURE_FLAGS      mFlags;
    ID3D12RootSignature            *mRootSignature;
};

INLINE void RootSignature::CleanupDescriptorRanges(D3D12_ROOT_PARAMETER &param) {
    if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE 
        && param.DescriptorTable.pDescriptorRanges) {
        delete param.DescriptorTable.pDescriptorRanges;
        param.DescriptorTable.pDescriptorRanges = nullptr;
    }
}

INLINE void RootSignature::SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    if (index >= mParameters.Count()) {
        return;
    }

    auto & param = mParameters.At(index);
    CleanupDescriptorRanges(param);

    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.ShaderVisibility = visibility;
    param.Constants.Num32BitValues = num32BitValues;
    param.Constants.ShaderRegister = shaderRegister;
    param.Constants.RegisterSpace = 0;
}

INLINE void RootSignature::SetDescriptor(uint32_t index, D3D12_ROOT_PARAMETER_TYPE type, uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    if (index >= mParameters.Count()) {
        return;
    }

    auto & param = mParameters.At(index);
    CleanupDescriptorRanges(param);

    param.ParameterType = type;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = 0;
}

INLINE void RootSignature::SetDescriptorTable(uint32_t index, D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t descCount, uint32_t baseRegister, D3D12_SHADER_VISIBILITY visibility) {
    if (index >= mParameters.Count()) {
        return;
    }

    auto & param = mParameters.At(index);
    CleanupDescriptorRanges(param);

    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = visibility;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = new CD3DX12_DESCRIPTOR_RANGE(rangeType, descCount, baseRegister, 0 /*register space*/);
}

}