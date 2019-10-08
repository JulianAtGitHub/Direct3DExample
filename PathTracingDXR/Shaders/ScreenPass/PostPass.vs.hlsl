struct VSInput {
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput {
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    return output;
}
