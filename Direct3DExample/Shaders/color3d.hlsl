cbuffer ConstBuffer : register(b0)
{
    float4x4 mvp;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 biNormal : BINORMAL;
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
    float4 newPos = float4(input.position, 1.0);
    newPos = mul(newPos, mvp);

    PSInput ret;
    ret.pos = newPos;
    ret.uv = input.texCoord;
    return ret;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
