
struct PSInput {
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

const static float exp = 0.4545454545f; // gamma correction 1.0 / 2.2

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = gTexture.SampleLevel(gSampler, input.uv, 0);
    return float4(pow(abs(color.rgb / color.w), exp), 1.0);
}

