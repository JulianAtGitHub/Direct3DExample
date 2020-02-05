#pragma once

class PbrDrawable;

class PbrPass {
public:
    enum State {
        NoIBLNoTex      = 0b00,
        NoIBLHasTex     = 0b01,
        HasIBLNoTex     = 0b10,
        HasIBLHasTex    = 0b11,

        FactorFNoTex    = HasIBLHasTex + 1,
        FactorFHasTex   = HasIBLHasTex + 2,
        FactorDNoTex    = HasIBLHasTex + 3,
        FactorDHasTex   = HasIBLHasTex + 4,
        FactorGNoTex    = HasIBLHasTex + 5,
        FactorGHasTex   = HasIBLHasTex + 6,

        StateMax
    };

    PbrPass(void);
    ~PbrPass(void);

    void PreviousRender(State state);
    void Render(uint32_t currentFrame, PbrDrawable *drawable, Render::DescriptorHeap *texHeap, uint32_t envIndex);

private:
    enum RootSignatureSlot {
        SettingsSlot = 0,
        TransformSlot,
        MaterialSlot,
        LightsSlot,
        IrradianceTexSlot,
        BlurredEnvTexSlot,
        BRDFLookupTexSlot,
        NormalTexSlot,
        AlbdoTexSlot,
        MetalnessTexSlot,
        RoughnessTexSlot,
        AOTexSlot,
        SamplerSlot,
        SlotCount
    };

    void Initialize(void);
    void Destroy(void);

    Render::RootSignature  *mRootSignature;
    Render::GraphicsState  *mGraphicsStates[StateMax];
    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler        *mSampler;
};
