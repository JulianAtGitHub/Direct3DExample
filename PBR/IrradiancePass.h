#pragma once

class IrradiancePass {
public:
    IrradiancePass(void);
    ~IrradiancePass(void);

    void Dispatch(Render::PixelBuffer *envTex, Render::PixelBuffer *irrTex);

private:
    void Initialize(void);
    void Destroy(void);

    enum RootSigSlot {
        ConstantSlot,
        EnvTexSlot,
        SamplerSlot,
        IrrTexSlot,
        RootSigSlotMax
    };

    Render::RootSignature  *mRootSignature;
    Render::ComputeState   *mPipelineState;
    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler        *mSampler;
};
