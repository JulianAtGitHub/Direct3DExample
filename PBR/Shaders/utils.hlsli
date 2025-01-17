
#include "constants.hlsli"

#ifndef _UTILS_HLSLI_
#define _UTILS_HLSLI_

//--------------- GGX D G F ---------------------

inline float DistributionGGX(float3 N, float3 H, float R) {
    float a = R * R;
    float a2 = a * a;
    float NDotH = saturate(dot(N, H));
    float NDotH2 = NDotH * NDotH;

    float denom = (NDotH2 * (a2 - 1.0f) + 1.0f);
    denom = M_PI * denom * denom;

    return a2 / denom;
}

inline float GeometryShlickGGX(float NdotV, float R) {
    float r = (R + 1.0f);
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

inline float GeometrySmith(float3 N, float3 V, float3 L, float R) {
    float NotV = max(dot(N, V), 0.0f);
    float NotL = max(dot(N, L), 0.0f);

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

float3 ImportanceSampleGGX(float2 Xi, float3 N, float R) {
    float a = R * R;

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

inline float GeometryShlickGGX_IBL(float NdotV, float R) {
    float k = (R * R) / 2.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

inline float GeometrySmith_IBL(float3 N, float3 V, float3 L, float R) {
    float NotV = max(dot(N, V), 0.0f);
    float NotL = max(dot(N, L), 0.0f);

    float ggx1 = GeometryShlickGGX_IBL(NotV, R);
    float ggx2 = GeometryShlickGGX_IBL(NotL, R);
    return ggx1 * ggx2;
}

//--------------- Hammersley ---------------------

// Hammersley sequence on hemi-sphere
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

inline float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // 0x100000000
}

inline float2 Hammersley2D(uint i, uint N) {
    return float2(float(i)/float(N), RadicalInverse_VdC(i));
}

//--------------- Spherical & Cartesian ---------------------

inline float2 DirToLatLong(float3 dir) {
    float3 p = normalize(dir);
    float u = (1.0f + atan2(p.x, -p.z) * M_1_PI) * 0.5f; // atan2 => [-PI, PI]
    float v = acos(p.y) * M_1_PI; //  acos => [0, PI]
    return float2(u, v);
}

inline float3 LatLongToDir(float2 ll) {
    ll = saturate(ll);
    float phi = 2.0f * M_PI * ll.x;
    float theta = M_PI * ll.y;

    // spherical coordinate to cartesian coordinate (right-hand)
    float3 dir = float3(-sin(theta) * sin(phi), cos(theta), sin(theta) * cos(phi));
    return normalize(dir);
}

//--------------- Others ---------------------

inline float3 LinearToSRGB(float3 x) {
    return x < 0.0031308f ? 12.92f * x : 1.055f * pow(abs(x), 1.0f / 2.4f) - 0.055f;
}
inline float3 SRGBToLinear(float3 x) {
    return x < 0.04045f ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
}

inline float3 LinearToSRGB_Opt(float3 x) {
    return pow(x, 0.454545454f);
}
inline float3 SRGBToLinear_Opt(float3 x) {
    return pow(x, 2.2f);
}

inline float3 TonemapReinhard(float3 color)
{
    color = color / (color + 1.0f);
    return LinearToSRGB_Opt(color);
}

// Output color space is SRGB 
inline float3 TonemapFilmic2(float3 color)
{
    float3 x = max(float3(0.0f, 0.0f, 0.0f), color - 0.004f);
    return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
}

#endif //_UTILS_HLSLI_
