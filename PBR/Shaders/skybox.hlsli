#pragma pack_matrix(row_major)

#include "constants.hlsli"

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_Position;
    float3 direction: TexCoord0;
};

cbuffer TransformCB : register(b0) {
    float4x4 ViewMat;
    float4x4 ProjMat;
};

PSInput VSMain(VSInput input) {
    PSInput ret;
    ret.direction = input.position;

    // remove transform component
    float4x4 rotViewMat = ViewMat;
    rotViewMat._41_42_43_44 = 0.0f;
    rotViewMat._14_24_34 = 0.0f;

    float4 clipPos = mul(mul(float4(input.position, 1.0f), rotViewMat), ProjMat);
    ret.position = clipPos.xyww;
    return ret;
}

Texture2D EnvTex : register(t0);

SamplerState Sampler : register(s0);

inline float2 DirToLatLong(float3 dir) {
    float3 p = normalize(dir);
    float u = (1.0f + atan2(p.x, -p.z) * M_1_PI) * 0.5f; // atan2 => [-PI, PI]
    float v = acos(p.y) * M_1_PI; //  acos => [0, PI]
    return float2(u, 1.0f - v);
}

float4 PSMain(PSInput input) : SV_TARGET {
    float2 uv = DirToLatLong(input.direction);
    float3 envColor = EnvTex.Sample(Sampler, uv).rgb;
    // HDR tonemap
    envColor = envColor / (envColor + 1.0f);
    // gamma correction
    envColor = pow(envColor, 1.0f / 2.2f);

    return float4(envColor.rgb, 1.0f);
}

