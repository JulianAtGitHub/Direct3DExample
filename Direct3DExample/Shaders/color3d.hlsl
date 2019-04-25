cbuffer ConstBuffer : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    float4 newPos = float4(input.pos, 1.0);
    newPos = mul(newPos, model);
    newPos = mul(newPos, view);
    newPos = mul(newPos, proj);

    PSInput ret;
    ret.pos = newPos;
    ret.uv = input.uv;
    return ret;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
