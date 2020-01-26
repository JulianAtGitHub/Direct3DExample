#pragma once

class BRDFIntegrationPass {
public:
    BRDFIntegrationPass(void);
    ~BRDFIntegrationPass(void);

    void Dispatch(Render::PixelBuffer *brdfTex);

private:
    void Initialize(void);
    void Destroy(void);

    enum RootSigSlot {
        ConstantSlot,
        BRDFTexSlot,
        RootSigSlotMax
    };

    Render::RootSignature  *mRootSignature;
    Render::ComputeState   *mPipelineState;
};
