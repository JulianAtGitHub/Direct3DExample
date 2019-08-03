#ifndef _RAYTRACING_COMMMON_H_
#define _RAYTRACING_COMMMON_H_

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

struct SceneConstants {
    float4 cameraPos;
    float4 cameraU;
    float4 cameraV;
    float4 cameraW;
    float4 bgColor;
    uint   frameCount;
    uint   accumCount;
    float  aoRadius;
};

struct PrimaryRayPayload {
    float4 color;
};

struct AORayPayload {
    float value;
};

namespace RayTraceParams {
    enum RayType {
        PrimaryRay = 0,
        AORay,
        RayTypeCount
    };

    static const uint InstanceMask = 0xff;   // Everything is visible.
    static const uint HitGroupIndex[RayTypeCount] = { 0, 1 };
    static const uint MissIndex[RayTypeCount] = { 0, 1 };

    static const float AlphaThreshold = 0.5f;
}

typedef BuiltInTriangleIntersectionAttributes Attributes;

/****Input & Output*****************************************/

RaytracingAccelerationStructure gRtScene : register(t0, space0);
// buffers
ByteAddressBuffer gIndices : register(t1, space0);
StructuredBuffer<Vertex> gVertices : register(t2, space0);
StructuredBuffer<Geometry> gGeometries : register(t3, space0);
Texture2D<float4> gMatTextures[] : register(t4);
// constant values
ConstantBuffer<SceneConstants> gSceneCB : register(b0);
// smapler
SamplerState gSampler : register(s0);
// output
RWTexture2D<float4> gRenderTarget : register(u0);
RWTexture2D<float4> gRenderDisplay : register(u1);


#endif // _RAYTRACING_COMMMON_H_