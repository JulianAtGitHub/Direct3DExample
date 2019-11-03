
cbuffer vertexBuffer : register(b0) {
    float4x4 ProjectionMatrix; 
};

struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv  : TEXCOORD;
    float4 col : COLOR;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
}
