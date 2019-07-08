#include "inputs.hlsli"

cbuffer ConstBuffer : register(b0)
{
    float4x4 mvp;
};

PSInput VSMain(VSInput input)
{
    float4 newPos = float4(input.position, 1.0);
    newPos = mul(newPos, mvp);

    PSInput ret;
    ret.pos = newPos;
    ret.uv = input.texCoord;
    return ret;
}
