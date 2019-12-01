#pragma once

class PbrDrawable;

class PbrPass {
public:
    enum RootSignatureSlot {
        SettingsSlot = 0,
        CameraSlot,
        MaterialSlot,
        LightsSlot,
        SlotCount
    };

    PbrPass(void);
    ~PbrPass(void);

    void PreviousRender(void);
    void Render(uint32_t currentFrame, PbrDrawable *drawable);

private:
    void Initialize(void);
    void Destroy(void);

    Render::RootSignature  *mRootSignature;
    Render::GraphicsState  *mGraphicsState;
    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler        *mSampler;
};
