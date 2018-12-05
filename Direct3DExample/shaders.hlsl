cbuffer ConstBuffer : register(b0)
{
    float4 offset;
};

struct VSInput
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input) 
{
    PSInput ret;
    ret.position = input.position + offset;
    ret.uv = input.uv;
    return ret;
}

float4 PSMain(PSInput input) : SV_TARGET 
{
    return g_texture.Sample(g_sampler, input.uv);
}
