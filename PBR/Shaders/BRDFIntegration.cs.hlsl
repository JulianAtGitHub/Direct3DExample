#include "utils.hlsli"

#define BLOCK_SIZE 8
#define SAMPLE_COUNT 1024

cbuffer CB0 : register(b0) {
    float2  TexelSize;      // 1.0 / OutTex.Dimensions
}

RWTexture2D<float2> OutTex  : register(u0);

float2 IntegrateBRDF(float NdotV, float roughness) {
    float3 V = float3(sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);
    float3 N = float3(0.0f, 0.0f, 1.0f);
    float2 ret = float2(0.0f, 0.0f);

    for(uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley2D(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0f);

        if(NdotL > 0.0) {
            float G = GeometrySmith_IBL(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0f - VdotH, 5.0f);

            ret += float2((1.0f - Fc) * G_Vis, Fc * G_Vis);
        }
    }
    ret /= float(SAMPLE_COUNT);

    return ret;
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(uint3 DTid : SV_DispatchThreadID) {
    float2 uv = TexelSize * (DTid.xy + 0.5f);
    OutTex[DTid.xy] = IntegrateBRDF(uv.x, uv.y);
}
