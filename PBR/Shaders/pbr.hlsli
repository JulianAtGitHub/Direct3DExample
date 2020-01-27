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

ConstantBuffer<SettingsCB>  Settings        : register(b0);
ConstantBuffer<TransformCB> Transform       : register(b1);
ConstantBuffer<MaterialCB>  Material        : register(b2);
StructuredBuffer<LightCB>   Lights          : register(t0);
Texture2D<float4>           IrradianceTex   : register(t1);
Texture2D<float4>           BlurredEnvTex   : register(t2);
Texture2D<float2>           BRDFLookupTex   : register(t3);
Texture2D<float4>           NormalTex       : register(t4);
Texture2D<float4>           AlbdoTex        : register(t5);
Texture2D<float4>           MetalnessTex    : register(t6);
Texture2D<float4>           RoughnessTex    : register(t7);
Texture2D<float4>           AOTex           : register(t8);
SamplerState                Sampler         : register(s0);

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
    float   metalness;
    float   roughness;
    float   ao;
};

void EvaluatePixel(in PSInput input, inout PixelSample ps) {
    if (Settings.enableTexture) {
        float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));
        float3 normal = NormalTex.Sample(Sampler, input.uv).rgb;
        normal = normalize(normal * 2.0f - 1.0f);
        ps.normal = normalize(mul(normal, TBN));

        ps.albdo = SRGBToLinear_Opt(AlbdoTex.Sample(Sampler, input.uv).rgb);
        ps.metalness = MetalnessTex.Sample(Sampler, input.uv).r;
        ps.roughness = RoughnessTex.Sample(Sampler, input.uv).r;
        ps.ao = AOTex.Sample(Sampler, input.uv).r;

    } else {
        ps.normal = normalize(input.normal);
        ps.albdo = Material.albdo;
        ps.metalness = Material.metalness;
        ps.roughness = Material.roughness;
        ps.ao = Material.ao;
    }
}

#define MAX_REFLECTION_LOD 4.0f

float4 PSMain(PSInput input) : SV_TARGET {
    PixelSample ps;
    EvaluatePixel(input, ps);

    float3 N = ps.normal;
    float3 V = normalize(Transform.cameraPos.xyz - input.worldPos);
    float NotV = max(dot(N, V), 0.0f);

    float3 F0 = lerp(F0_MIN, ps.albdo, float3(ps.metalness, ps.metalness, ps.metalness));

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
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
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 Ks = F;
        float3 Kd = float3(1.0f, 1.0f, 1.0f) - Ks;
        // pure matel do not have refraction
        Kd *= 1.0f - ps.metalness;

        float NotL = max(dot(N, L), 0.0f);
        float3 specular = (D * G * F) / max(4.0f * NotV * NotL, 0.001f);

        Lo += (Kd * ps.albdo * M_1_PI + specular) * radiance * NotL;
    }

    float3 color = Lo;

    // environment color
    if (Settings.enableIBL) {
        float3 Ks = FresnelSchlickRoughness(saturate(dot(N, V)), F0, ps.roughness);
        float3 Kd = float3(1.0f, 1.0f, 1.0f) - Ks;
        Kd *= 1.0f - ps.metalness;

        float3 irradiance = IrradianceTex.SampleLevel(Sampler, DirToLatLong(N), 0).rgb;
        float3 diffuse = irradiance * ps.albdo;

        float3 R = reflect(-V, N);
        float3 blurredEnvColor = BlurredEnvTex.SampleLevel(Sampler, DirToLatLong(R), MAX_REFLECTION_LOD * ps.roughness).rgb;
        float2 envBRDF = BRDFLookupTex.SampleLevel(Sampler, float2(NotV, ps.roughness), 0).rg;
        float3 specular = blurredEnvColor * (Ks * envBRDF.x + envBRDF.y);

        float3 ambient = (Kd * diffuse + specular) * ps.ao;
        color += ambient;
    } else {
        float3 ambient = float3(0.03f, 0.03f, 0.03f) * ps.albdo * ps.ao;
        color += ambient;
    }

    // HDR tonemap
    color = TonemapReinhard(color);

    return float4(color, 1.0f);
}




