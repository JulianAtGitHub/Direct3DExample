#ifndef _TYPES_PBR_H_
#define _TYPES_PBR_H_

#ifdef __cplusplus
#define float4 XMFLOAT4
#define float3 XMFLOAT3
#define float4x4 XMFLOAT4X4
#define uint uint32_t
#endif

struct SettingsCB {
    uint    numLight;
    uint    brdfIndex;
    uint    envTexCount;
    uint    envIndex;
};

struct TransformCB {
    float4x4 model;
    float4x4 mvp;
    float4   cameraPos;
};

struct MatValuesCB {
    float3  basic;
    float   metallic;
    float   roughness;
    float   occlusion;
    float3  ambient;
    float3  emissive;
    uint    matIndex;
};

struct LightCB {
    float3  position;
    uint    type;
    float3  color;
    float   angle;
};

struct MaterialCB {
    float   normalScale;
    uint    normalTexture;
    float   occlusionStrength;
    uint    occlusionTexture;
    float3  emissiveFactor;
    uint    emissiveTexture;
    float4  baseFactor;
    uint    baseTexture;
    float3  reserved;   // unused
    float   metallicFactor;
    uint    metallicTexture;
    float   roughnessFactor;
    uint    roughnessTexture;
};

#endif // _TYPES_PBR_H_
