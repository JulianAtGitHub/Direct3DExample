cbuffer ConstBuffer : register(b0)
{
    float4x4 mvp;
};

struct VSInput
{
    float4 pos : POSITION;
};

struct PSInput
{
    float4 pos : SV_POSITION;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput ret;
    ret.pos = mul(input.pos, mvp);
    return ret;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1.0, 0.0, 1.0, 1.0);
}
