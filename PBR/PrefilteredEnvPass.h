#pragma once

class PrefilteredEnvPass {
public:
    PrefilteredEnvPass(void);
    ~PrefilteredEnvPass(void);

    void Dispatch(Render::PixelBuffer *envTex, Render::PixelBuffer *blurredTex);

private:
    void Initialize(void);
    void Destroy(void);

    enum RootSigSlot {
        ConstantSlot,
        EnvTexSlot,
        SamplerSlot,
        BlurredTexSlot,
        RootSigSlotMax
    };

    Render::RootSignature  *mRootSignature;
    Render::ComputeState   *mPipelineState;
    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler        *mSampler;
};
