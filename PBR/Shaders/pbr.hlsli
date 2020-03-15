#pragma pack_matrix(row_major)

#include "types.pbr.h"
#include "utils.hlsli"

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

ConstantBuffer<SettingsCB>      Settings    : register(b0);
ConstantBuffer<TransformCB>     Transform   : register(b1);
ConstantBuffer<MatValuesCB>     MatValues   : register(b2);
StructuredBuffer<LightCB>       Lights      : register(t0);
StructuredBuffer<MaterialCB>    Materials   : register(t1);
Texture2D<float4>               MatTexs[]   : register(t2);
// change to space1 to avoid resource range overlap
Texture2D<float4>               EnvTexs[]   : register(t3, space1);
SamplerState                    Sampler     : register(s0);
SamplerState                    SamplerEnv  : register(s1, space1);

PSInput VSMain(VSInput input) {
    PSInput ret;

    ret.position = mul(float4(input.position, 1.0f), Transform.mvp);
    ret.worldPos = mul(float4(input.position, 1.0f), Transform.model).xyz;
    ret.uv = input.texCoord;
    ret.normal = mul(input.normal, (float3x3)Transform.model);
    ret.tangent = mul(input.tangent, (float3x3)Transform.model);
    ret.bitangent = mul(input.bitangent, (float3x3)Transform.model);

    return ret;
}

struct PixelSample {
    float3  normal;
    float3  albdo;
    float   metallic;
    float   roughness;
    float   ocllusion;
    float3  emissive;
};

void EvaluatePixel(in PSInput input, inout PixelSample ps) {

#ifdef ENABLE_TEXTURE
    MaterialCB mat = Materials[MatValues.matIndex];

    float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));
    float3 normal = MatTexs[mat.normalTexture].Sample(Sampler, input.uv).rgb;
    normal = normalize((normal * 2.0f - 1.0f) * float3(mat.normalScale, mat.normalScale, 1.0f));
    ps.normal = normalize(mul(normal, TBN));

    ps.albdo = (mat.baseTexture != TEX_INDEX_INVALID ? SRGBToLinear_Opt(MatTexs[mat.baseTexture].Sample(Sampler, input.uv).rgb) * mat.baseFactor.rgb : mat.baseFactor.rgb);
    ps.metallic = (mat.metallicTexture != TEX_INDEX_INVALID ? MatTexs[mat.metallicTexture].Sample(Sampler, input.uv).b * mat.metallicFactor : mat.metallicFactor);
    ps.roughness = (mat.roughnessTexture != TEX_INDEX_INVALID ? MatTexs[mat.roughnessTexture].Sample(Sampler, input.uv).g * mat.roughnessFactor : mat.roughnessFactor);
    ps.ocllusion = (mat.occlusionTexture != TEX_INDEX_INVALID ? MatTexs[mat.occlusionTexture].Sample(Sampler, input.uv).r * mat.occlusionStrength : mat.occlusionStrength);
    ps.emissive = (mat.emissiveTexture != TEX_INDEX_INVALID ? SRGBToLinear_Opt(MatTexs[mat.emissiveTexture].Sample(Sampler, input.uv).rgb) * mat.emissiveFactor : mat.emissiveFactor);

#else
    ps.normal = normalize(input.normal);
    ps.albdo = MatValues.basic;
    ps.metallic = MatValues.metallic;
    ps.roughness = MatValues.roughness;
    ps.ocllusion = MatValues.occlusion;
    ps.emissive = MatValues.emissive;

#endif
}

#define MAX_REFLECTION_LOD 4.0f

float4 PSMain(PSInput input) : SV_TARGET {
    PixelSample ps;
    EvaluatePixel(input, ps);

    float3 N = ps.normal;
    float3 V = normalize(Transform.cameraPos.xyz - input.worldPos);
    float NotV = max(dot(N, V), 0.0f);

    float3 F0 = lerp(F0_MIN, ps.albdo, float3(ps.metallic, ps.metallic, ps.metallic));

    float3 color = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < Settings.numLight; ++i) {
        // calculate light radiance
        float3 L = normalize(Lights[i].position - input.worldPos);
        float3 H = normalize(V + L);
        float distance = length(Lights[i].position - input.worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = Lights[i].color * attenuation;

        // cook-torrance brdf
        float D = DistributionGGX(N, H, ps.roughness);
        float G = GeometrySmith(N, V, L, ps.roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

        float3 Ks = F;
        float3 Kd = float3(1.0f, 1.0f, 1.0f) - Ks;
        // pure matel do not have refraction
        Kd *= 1.0f - ps.metallic;

        float NotL = max(dot(N, L), 0.0f);
        float3 specular = (D * G * F) / max(4.0f * NotV * NotL, 0.001f);

        color += (Kd * ps.albdo * M_1_PI + specular) * radiance * NotL;
    }

    // environment color
#ifdef ENABLE_IBL
    float3 Ks = FresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, ps.roughness);
    float3 Kd = float3(1.0f, 1.0f, 1.0f) - Ks;
    Kd *= 1.0f - ps.metallic;

    uint irrTexIdx = Settings.envTexCount * Settings.envIndex + 1;
    float3 irradiance = EnvTexs[irrTexIdx].SampleLevel(SamplerEnv, DirToLatLong(N), 0).rgb;
    float3 diffuse = irradiance * ps.albdo;

    float3 R = reflect(-V, N);
    uint blurredTexIdx = Settings.envTexCount * Settings.envIndex + 2;
    float3 blurredEnvColor = EnvTexs[blurredTexIdx].SampleLevel(SamplerEnv, DirToLatLong(R), MAX_REFLECTION_LOD * ps.roughness).rgb;
    float2 envBRDF = EnvTexs[Settings.brdfIndex].SampleLevel(SamplerEnv, float2(NotV, ps.roughness), 0).rg;
    float3 specular = blurredEnvColor * (Ks * envBRDF.x + envBRDF.y);

    float3 ambient = (Kd * diffuse + specular) * ps.ocllusion;

#else
    float3 ambient = MatValues.ambient * ps.albdo * ps.ocllusion;

#endif

    color += ambient;
    color += ps.emissive;

    // HDR tonemap
    color = TonemapReinhard(color);

    return float4(color, 1.0f);
}

#ifdef SEPERATR_COMPONENT

float4 PSMain_F(PSInput input) : SV_TARGET {
    PixelSample ps;
    EvaluatePixel(input, ps);

    float3 N = ps.normal;
    float3 V = normalize(Transform.cameraPos.xyz - input.worldPos);
    float3 F0 = lerp(F0_MIN, ps.albdo, float3(ps.metallic, ps.metallic, ps.metallic));
    float3 L = normalize(Lights[0].position - input.worldPos);
    float3 H = normalize(V + L);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float NotV = max(dot(N, V), 0.0f);
    float NotL = max(dot(N, L), 0.0f);

    F = (F / max(4.0f * NotV * NotL, 0.001f)) * NotL;

    return float4(F, 1.0f);
}

float4 PSMain_D(PSInput input) : SV_TARGET {
    PixelSample ps;
    EvaluatePixel(input, ps);

    float3 N = ps.normal;
    float3 V = normalize(Transform.cameraPos.xyz - input.worldPos);
    float3 L = normalize(Lights[0].position - input.worldPos);
    float3 H = normalize(V + L);
    float D = DistributionGGX(N, H, ps.roughness);

    float NotL = max(dot(N, L), 0.0f);
    D *= NotL;

    return float4(D, D, D, 1.0f);
}

float4 PSMain_G(PSInput input) : SV_TARGET {
    PixelSample ps;
    EvaluatePixel(input, ps);

    float3 N = ps.normal;
    float3 V = normalize(Transform.cameraPos.xyz - input.worldPos);
    float3 L = normalize(Lights[0].position - input.worldPos);
    float G = GeometrySmith(N, V, L, ps.roughness);

    float NotL = max(dot(N, L), 0.0f);
    G *= NotL;

    return float4(G, G, G, 1.0f);
}

#endif



