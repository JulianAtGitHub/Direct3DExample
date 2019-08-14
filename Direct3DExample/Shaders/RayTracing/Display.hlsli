struct VSInput {
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput {
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

PSInput VSMain(VSInput input) {
    PSInput output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 color = gTexture.SampleLevel(gSampler, input.uv, 0).rgb;
    return float4(color, 1.0);
}
