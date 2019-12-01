#pragma pack_matrix(row_major)

#include "types.pbr.h"

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 bitangent: BITANGENT;
};

struct PSInput {
    float4 position : SV_Position;
    float3 worldPos : Position;
    float2 uv       : TexCoord0;
    float3 normal   : Normal;
    float3 tangent  : Tangent;
    float3 bitangent: Bitangent;
};

ConstantBuffer<SettingsCB>  Settings    : register(b0);
ConstantBuffer<CameraCB>    Camera      : register(b1);
ConstantBuffer<MaterialCB>  Material    : register(b2);
StructuredBuffer<LightCB>   Lights      : register(t0, space0);

static const float M_PI = 3.14159265359f;
static const float3 F0_MIN = float3(0.04f, 0.04f, 0.04f);

//--------------- D G F ---------------------

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

//--------------- Shader Main ---------------------

PSInput VSMain(VSInput input) {
    PSInput ret;

    ret.position = mul(float4(input.position, 1.0f), Camera.mvp);
    ret.worldPos = input.position;
    ret.uv = input.texCoord;
    ret.normal = input.normal;
    ret.tangent = input.tangent;
    ret.bitangent = input.bitangent;

    return ret;
}

#ifdef PBR_PS

//Texture2D Tex : register(t0);
//SamplerState Sampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET {
    // return Tex.Sample(Sampler, input.uv);

    float3 N = normalize(input.normal);
    float3 V = normalize(Camera.position.xyz - input.worldPos);

    float3 F0 = lerp(F0_MIN, Material.albdo, Material.metalness);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < Settings.numLight; ++i) {
        // calculate light radiance
        float3 L = normalize(Lights[i].position - input.worldPos);
        float3 H = normalize(V + L);
        float distance = length(Lights[i].position - input.worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = Lights[i].color * attenuation;

        // cook-torrance brdf
        float D = DistributionGGX(N, H, Material.roughness);
        float G = GeometrySmith(N, V, L, Material.roughness);
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 Ks = F;
        float3 Kd = float3(1.0f, 1.0f, 1.0f) - Ks;
        // pure matel do not have refraction
        Kd *= 1.0f - Material.metalness;

        float NotV = saturate(dot(N, V));
        float NotL = saturate(dot(N, L));
        float3 specular = (D * G * F) / max(4.0f * NotV * NotL, 0.001f);
        float3 brdf = Kd * Material.albdo / M_PI + specular;

        Lo += (brdf * radiance * NotL);
    }

    float3 ambient = float3(0.03f, 0.03f, 0.03f) * Material.albdo * Material.ao;
    float3 color = ambient + Lo;

    // HDR tonemap
    color = color / (color + 1.0f);
    // gamma correction
    color = pow(color, 1.0f / 2.2f);

    return float4(color, 1.0f);
}

#endif



