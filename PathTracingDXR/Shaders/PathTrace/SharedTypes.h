#ifndef _PATHTRACE_SHAREDTYPES_H_
#define _PATHTRACE_SHAREDTYPES_H_

#ifdef __cplusplus
#define float4 XMFLOAT4
#define float3 XMFLOAT3
#define float2 XMFLOAT2
#define float4x4 XMFLOAT4X4
#define uint4 XMUINT4 
#define uint uint32_t 
#endif

struct Vertex {
    float3 position;
    float2 texCoord;
    float3 normal;
    float3 tangent;
    float3 bitangent;
};

struct Geometry {
    uint    indexOffset;
    uint    indexCount;
    uint    reserve0;
    uint    reserve1;
};

// material type
enum MatType {
    LambertianMat,
    MetalMat,
    DielectricMat,
    MatTypeCount
};

struct Material {
    uint    type;
    uint    normalTex;
    uint    albedoTex;
    uint    roughnessTex;
    float4  albedoColor;
    float3  emissiveColor;
    float   roughness;
    float   refractivity;
};

struct Light {
    uint   type;
    float  openAngle;
    float  penumbraAngle;
    float  cosOpenAngle;
    float3 position;
    float3 direction;
    float3 intensity;
};

struct CameraConstants {
    float4 position;
    float4 u;
    float4 v;
    float4 w;
    float2 jitter;
    float  lensRadius;
    float  focalLength;
};

struct SceneConstants {
    float4 bgColor;
    uint   lightCount;
    uint   frameSeed;
    uint   accumCount;
    uint   maxRayDepth;
    uint   sampleCount;
};

struct AppSettings {
    uint enableAccumulate;
    uint enableJitterCamera;
    uint enableLensCamera;
    uint enableEnvironmentMap;
};

#endif // _PATHTRACE_SHAREDTYPES_H_