// Ref from: https://www.3dgep.com/learning-directx-12-4/#Generate_Mips_Pipeline_State_Object

#define BLOCK_SIZE 8

cbuffer CB0 : register(b0) {
    uint    SrcMipLevel;
    uint    OutMipsCount;   // [1, 4]
    float2  TexelSize;      // 1.0 / OutMip1.Dimensions
}

Texture2D<float4> SrcMip : register(t0);
SamplerState Sampler : register(s0);
RWTexture2D<float4> OutMip1 : register(u0);
RWTexture2D<float4> OutMip2 : register(u1);
RWTexture2D<float4> OutMip3 : register(u2);
RWTexture2D<float4> OutMip4 : register(u3);

// The reason for separating channels is to reduce bank conflicts in the local data memory controller.
// A large stride will cause more threads to collide on the same memory bank.
groupshared float gsR[64];
groupshared float gsG[64];
groupshared float gsB[64];
groupshared float gsA[64];

inline void StoreColor(uint i, float4 c) {
    gsR[i] = c.r;
    gsG[i] = c.g;
    gsB[i] = c.b;
    gsA[i] = c.a;
}

inline float4 LoadColor(uint i) {
    return float4(gsR[i], gsG[i], gsB[i], gsA[i]);
}

// ref from: https://en.wikipedia.org/wiki/SRGB
inline float3 LinearToSRGB(float3 x) {
    return x < 0.0031308f ? 12.92f * x : 1.055f * pow(abs(x), 0.416666667f) - 0.055f; // 1.0 / 2.4 = 0.416666667
}
inline float3 SRGBToLinear(float3 x) {
    return x < 0.04045f ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
}

inline float4 PackColor(float4 c) {
#ifdef CONVERT_TO_SRGB
    return float4(LinearToSRGB(c.rgb), c.a);
#else
    return c;
#endif
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID) {
    // One bilinear sample is insufficient when scaling down by more than 2x.
    // You will slightly undersample in the case where the source dimension is odd.
    // This is why it's a really good idea to only generate mips on power-of-two sized textures.
    // Trying to handle the undersampling case will force this shader to be slower and more complicated as it will have to take more source texture samples.

#if defined WIDTH_EVEN_HEIGHT_EVEN
    float2 UV = TexelSize * (DTid.xy + 0.5f);
    float4 Src1 = SrcMip.SampleLevel(Sampler, UV, SrcMipLevel);

#elif defined WIDTH_ODD_HEIGHT_EVEN
    // > 2:1 in X dimension
    // Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x horizontally.
    float2 UV1 = TexelSize * (DTid.xy + float2(0.25f, 0.5f));
    float2 Off = TexelSize * float2(0.5f, 0.0f);
    float4 Src1 = 0.5f * (SrcMip.SampleLevel(Sampler, UV1, SrcMipLevel)
                  + SrcMip.SampleLevel(Sampler, UV1 + Off, SrcMipLevel));

#elif defined WIDTH_EVEN_HEIGHT_ODD
    // > 2:1 in Y dimension
    // Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x vertically.
    float2 UV1 = TexelSize * (DTid.xy + float2(0.5f, 0.25f));
    float2 Off = TexelSize * float2(0.0f, 0.5f);
    float4 Src1 = 0.5 * (SrcMip.SampleLevel(Sampler, UV1, SrcMipLevel)
                  + SrcMip.SampleLevel(Sampler, UV1 + Off, SrcMipLevel));

#elif defined WIDTH_ODD_HEIGHT_ODD
    // > 2:1 in in both dimensions
    // Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x in both directions.
    float2 UV1 = TexelSize * (DTid.xy + float2(0.25f, 0.25f));
    float2 O = TexelSize * 0.5f;
    float4 Src1 = SrcMip.SampleLevel(Sampler, UV1, SrcMipLevel);
    Src1 += SrcMip.SampleLevel(Sampler, UV1 + float2(O.x, 0.0), SrcMipLevel);
    Src1 += SrcMip.SampleLevel(Sampler, UV1 + float2(0.0, O.y), SrcMipLevel);
    Src1 += SrcMip.SampleLevel(Sampler, UV1 + float2(O.x, O.y), SrcMipLevel);
    Src1 *= 0.25f;
#endif

    OutMip1[DTid.xy] = PackColor(Src1);

    // A scalar (constant) branch can exit all threads coherently.
    if (OutMipsCount == 1) {
        return;
    }

    // Without lane swizzle operations, the only way to share data with other
    // threads is through LDS.
    StoreColor(GI, Src1);

    // This guarantees all LDS writes are complete and that all threads have
    // executed all instructions so far (and therefore have issued their LDS
    // write instructions.)
    GroupMemoryBarrierWithGroupSync();

    // With low three bits for X and high three bits for Y, this bit mask
    // (binary: 001001) checks that X and Y are even.
    if ((GI & 0x9) == 0) {
        float4 Src2 = LoadColor(GI + 0x01);
        float4 Src3 = LoadColor(GI + 0x08);
        float4 Src4 = LoadColor(GI + 0x09);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

        OutMip2[DTid.xy / 2] = PackColor(Src1);
        StoreColor(GI, Src1);
    }

    if (OutMipsCount == 2) {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // This bit mask (binary: 011011) checks that X and Y are multiples of four.
    if ((GI & 0x1B) == 0) {
        float4 Src2 = LoadColor(GI + 0x02);
        float4 Src3 = LoadColor(GI + 0x10);
        float4 Src4 = LoadColor(GI + 0x12);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

        OutMip3[DTid.xy / 4] = PackColor(Src1);
        StoreColor(GI, Src1);
    }

    if (OutMipsCount == 3) {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // This bit mask would be 111111 (X & Y multiples of 8), but only one
    // thread fits that criteria.
    if (GI == 0) {
        float4 Src2 = LoadColor(GI + 0x04);
        float4 Src3 = LoadColor(GI + 0x20);
        float4 Src4 = LoadColor(GI + 0x24);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

        OutMip4[DTid.xy / 8] = PackColor(Src1);
    }
}