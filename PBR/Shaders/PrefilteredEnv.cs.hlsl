#include "utils.hlsli"

#define BLOCK_SIZE 8
#define SAMPLE_COUNT 1024

cbuffer CB0 : register(b0) {
    float2  TexelSize;      // 1.0 / OutTex.Dimensions
    float   Roughness;
}

Texture2D<float4>   EnvTex  : register(t0);
SamplerState        Sampler : register(s0);
RWTexture2D<float4> OutTex  : register(u0);

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0f * M_PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // spherical coordinates to cartesian coordinates in tangent space
    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    // tangent space to world space
    float3 sampleDir = tangent * H.x + bitangent * H.y + N * H.z;

    return normalize(sampleDir);
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(uint3 DTid : SV_DispatchThreadID) {
    float2 uv = TexelSize * (DTid.xy + 0.5f);
    float3 N = LatLongToDir(uv);
    float3 R = N;
    float3 V = R;

    float totalWeight = 0.0f;
    float3 prefilteredColor = float3(0.0f, 0.0f, 0.0f);
    for(uint i = 0; i < SAMPLE_COUNT; ++i) {
        float2 Xi = Hammersley2D(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, Roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0f);
        if(NdotL > 0.0f) {
            prefilteredColor += EnvTex.SampleLevel(Sampler, DirToLatLong(L), 0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    OutTex[DTid.xy] = float4(prefilteredColor, 1.0f);
}