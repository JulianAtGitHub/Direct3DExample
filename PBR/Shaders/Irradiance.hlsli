
#include "constants.hlsli"

cbuffer CB0 : register(b0) {
    float2  TexelSize;      // 1.0 / OutMip1.Dimensions
}

Texture2D<float4>   EnvTex  : register(t0);
SamplerState        Sampler : register(s0);
RWTexture2D<float4> OutTex  : register(u0);

inline float2 DirToLatLong(float3 dir) {
    float3 p = normalize(dir);
    float u = (1.0f + atan2(p.x, -p.z) * M_1_PI) * 0.5f; // atan2 => [-PI, PI]
    float v = acos(p.y) * M_1_PI; //  acos => [0, PI]
    return float2(u, 1.0f - v);
}

inline float3 LatLongToDir(float2 ll) {
    ll = saturate(ll);
    float3 dir;
    dir.y = 1.0f - 2.0f * ll.y;
    dir.x = -sin(ll.x * M_PI * 2.0f);
    dir.z = cos(ll.x * M_PI * 2.0f);
    return normalize(dir);
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(uint3 DTid : SV_DispatchThreadID) {
    float2 uv = TexelSize * (DTid.xy + 0.5f);
    float3 normal = LatLongToDir(uv);

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);

    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 right = cross(up, normal);
    up = cross(normal, right);

    float delta = 0.025f;
    float count = 0.0f;

    for (float phi = 0.0f; phi < 2 * M_PI; phi += delta) {
        for (float theta = 0.0f; theta < M_PI_2; theta += delta) {
            // spherical to cartesian (in tangent space)
            float3 tangent = float3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            float3 sampleDir = tangent.x * right + tangent.y * up + tangent.z * normal;

            irradiance += EnvTex.SampleLevel(Sampler, DirToLatLong(sampleDir), 0).rgb * cos(theta) * sin(theta);
            count += 1.0f;
        }
    }

    irradiance = M_PI * irradiance / count;

    OutTex[DTid.xy] = float4(irradiance, 1.0f);
}
