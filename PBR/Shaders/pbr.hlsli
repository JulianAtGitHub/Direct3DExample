#pragma pack_matrix(row_major)

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 bitangent: BITANGENT;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 uv       : TexCoord0;
    float3 normal   : Normal;
    float3 tangent  : Tangent;
    float3 bitangent: Bitangent;
};

cbuffer TransformCB : register(b0) {
    float4x4 MVP;
};

PSInput VSMain(VSInput input) {
    PSInput ret;

    ret.position = mul(float4(input.position, 1.0f), MVP);
    ret.uv = input.texCoord;
    ret.normal = input.normal;
    ret.tangent = input.tangent;
    ret.bitangent = input.bitangent;

    return ret;
}

#ifdef PBR_PS

//Texture2D Tex : register(t0);
//SamplerState Sampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET {
    // return Tex.Sample(Sampler, input.uv);
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

#endif



