
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
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD;
};
