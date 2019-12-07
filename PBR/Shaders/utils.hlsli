
#include "constants.hlsli"

#ifndef _UTILS_HLSLI_
#define _UTILS_HLSLI_

//--------------- Spherical & Cartesian ---------------------

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

//--------------- GGX D G F ---------------------

float DistributionGGX(float3 N, float3 H, float R) {
    float a = R * R;
    float a2 = a * a;
    float NDotH = saturate(dot(N, H));
    float NDotH2 = NDotH * NDotH;

    float denom = (NDotH2 * (a2 - 1.0f) + 1.0f);
    denom = M_PI * denom * denom;

    return a2 / denom;
}

inline float GeometryShlickGGX(float NdotV, float R) {
#ifdef ENABLE_IBL
    float k = (R * R) / 2.0f;
#else
    float r = (R + 1.0f);
    float k = (r * r) / 8.0f;
#endif
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float R) {
    float NotV = saturate(dot(N, V));
    float NotL = saturate(dot(N, L));

    float ggx1 = GeometryShlickGGX(NotV, R);
    float ggx2 = GeometryShlickGGX(NotL, R);
    return ggx1 * ggx2;
}

inline float3 FresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * pow(1.0f - cosTheta, 5.0f);
}

inline float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float R) {
    float3 FR = float3(1.0f - R, 1.0f - R, 1.0f - R);
    return F0 + (max(FR, F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}

//--------------- Others ---------------------

inline float3 SRGBToLinear(float3 x) {
    return x < 0.04045f ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
}

#endif //_UTILS_HLSLI_
