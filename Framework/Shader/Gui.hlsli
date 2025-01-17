
cbuffer CB0 : register(b0) {
    float4x4 ProjectionMatrix; 
};

cbuffer CB1 : register(b1) {
    float MipSlice; 
};

SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);

struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv  : TEXCOORD;
    float4 col : COLOR;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

PS_INPUT VSMain(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_Target {
    float4 out_col = input.col * texture0.SampleLevel(sampler0, input.uv, MipSlice); 
    return out_col;
}
