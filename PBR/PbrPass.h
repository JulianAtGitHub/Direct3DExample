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

    const static uint32_t ENV_TEX_MAX = 16;
    const static uint32_t MAT_TEX_MAX = 16;

    PbrPass(void);
    ~PbrPass(void);

    void PreviousRender(State state);
    void Render(uint32_t currentFrame, PbrDrawable *drawable);

private:
    enum RootSignatureSlot {
        SettingsSlot = 0,
        TransformSlot,
        MatValuesSlot,
        LightsSlot,
        MaterialsSlot,
        MatTexsSlot,
        EnvTexsSlot,
        SamplerSlot,
        SamplerEnvSlot,
        SlotCount
    };

    void Initialize(void);
    void Destroy(void);

    Render::RootSignature  *mRootSignature;
    Render::GraphicsState  *mGraphicsStates[StateMax];
    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler        *mSampler;
    Render::Sampler        *mSamplerEnv;
};
