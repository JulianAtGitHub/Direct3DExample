#ifndef _RAYTRACING_COMMMON_H_
#define _RAYTRACING_COMMMON_H_

struct AppSettings {
    uint enableAccumulate;
    uint enableJitterCamera;
    uint enableLensCamera;
    uint enableEnvironmentMap;
};

struct Vertex {
    float3 position;
    float2 texCoord;
    float3 normal;
    float3 tangent;
    float3 bitangent;
};

struct Geometry {
    uint4 indexInfo; // x: index offset, y: index count;
    uint4 texInfo;  // x: diffuse, y: specular, z: normal
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

struct LightSample {
    float3 L;
    float3 diffuse;
    float3 specular;
    float3 position;
};

struct CameraConstants {
    float3 pos;
    float3 u;
    float3 v;
    float3 w;
    float2 jitter;
    float  lensRadius;
    float  focalLength;
};

struct SceneConstants {
    float4 bgColor;
    uint   lightCount;
    uint   frameCount;
    uint   accumCount;
    float  aoRadius;
};

struct PrimaryRayPayload {
    float4 color;
    uint randSeed;
};

struct RayHitPayload {
    float value;
};

typedef RayHitPayload AORayPayload;

typedef RayHitPayload ShadowRayPayload;

typedef BuiltInTriangleIntersectionAttributes Attributes;

static const float M_PI = 3.14159265f;
static const float M_1_PI = 0.318309886f;
static const float FLT_MAX = 3.402823466e+38f;

namespace RayTraceParams {
    enum RayType {
        PrimaryRay = 0,
        AORay,
        ShadowRay,
        RayTypeCount
    };

    enum LightType {
        DirectLight = 0,
        PointLight,     // include spot light
        LightTypeCount
    };

    static const uint InstanceMask = 0xff;   // Everything is visible.
    static const uint HitGroupIndex[RayTypeCount] = { 0, 1, 2 };
    static const uint MissIndex[RayTypeCount] = { 0, 1, 2 };

    static const float AlphaThreshold = 0.5f;
}

/****Input & Output*****************************************/

RaytracingAccelerationStructure gRtScene : register(t0, space0);
// buffers
ByteAddressBuffer gIndices : register(t1, space0);
StructuredBuffer<Vertex> gVertices : register(t2, space0);
StructuredBuffer<Geometry> gGeometries : register(t3, space0);
StructuredBuffer<Light> gLights : register(t4, space0);
Texture2D<float4> gEnvTexture : register(t5);
Texture2D<float4> gMatTextures[] : register(t6);
// constant values
ConstantBuffer<AppSettings> gSettingsCB : register(b0);
ConstantBuffer<SceneConstants> gSceneCB : register(b1);
ConstantBuffer<CameraConstants> gCameraCB : register(b2);
// smapler
SamplerState gSampler : register(s0);
// output
RWTexture2D<float4> gRenderTarget : register(u0);
RWTexture2D<float4> gRenderDisplay : register(u1);


#endif // _RAYTRACING_COMMMON_H_
