
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);

float4 main(PS_INPUT input) : SV_Target {
    float4 out_col = input.col * texture0.SampleLevel(sampler0, input.uv, 0); 
    return out_col;
}
