#include "inputs.hlsli"

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.norm, 1.0);
    // return g_texture.Sample(g_sampler, input.uv);
}
