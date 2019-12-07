#pragma once

namespace Render {
    class RootSignature;
    class ComputeState;
    class DescriptorHeap;
    class Sampler;
    class PixelBuffer;
}

namespace Utils {

class MipsGenerator {
public:
    MipsGenerator(void);
    ~MipsGenerator(void);

    void Dispatch(Render::PixelBuffer *resource);

private:
    void Initialize(void);
    void Destroy(void);

    enum ColorSpace {
        LinearSpace,
        SRGBSpace,
        ColorSpaceMax
    };

    enum SizeType {
        X_EVEN_Y_EVEN,
        X_ODD_Y_EVEN,
        X_EVEN_Y_ODD,
        X_ODD_Y_ODD,
        SizeTypeMax
    };

    enum RootSigSlot {
        ConstantSlot,
        SourceTexSlot,
        SamplerSlot,
        OutputsSlot,
        RootSigSlotMax
    };

    Render::RootSignature  *mRootSignature;
    Render::ComputeState   *mPipelineState[ColorSpaceMax][SizeTypeMax];
    Render::DescriptorHeap *mSamplerHeap;
    Render::Sampler        *mSampler;
};

extern MipsGenerator  *gMipsGener;
extern void CreateMipsGenerator(void);
extern void DestroyMipsGenerator(void);

}
