#ifndef _RAYTRACING_RAYGENER_H_
#define _RAYTRACING_RAYGENER_H_

#include "Common.hlsli"

inline void PinholdCameraRay(in float2 pixel, out RayDesc ray) {
    float2 ndc = float2(2, -2) * pixel + float2(-1, 1);
    ray.Origin = gCameraCB.position;
    ray.Direction = normalize(ndc.x * gCameraCB.u + ndc.y * gCameraCB.v + gCameraCB.w);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;
}

inline void LensCameraRay(in float2 pixel, inout uint randSeed, out RayDesc ray) {
    float2 ndc = float2(2, -2) * pixel + float2(-1, 1);

    float3 rayDir = ndc.x * gCameraCB.u + ndc.y * gCameraCB.v + gCameraCB.w;
    rayDir /= length(gCameraCB.w);

    float3 focalPoint = gCameraCB.position + gCameraCB.focalLength * rayDir;

    // Get point on lens (in polar coords then convert to Cartesian)
    float2 rnd = float2(2.0f * M_PI * NextRand(randSeed),  gCameraCB.lensRadius * NextRand(randSeed));
    float2 uv = float2(cos(rnd.x) * rnd.y, sin(rnd.x) * rnd.y);

    ray.Origin = gCameraCB.position + uv.x * normalize(gCameraCB.u) + uv.y * normalize(gCameraCB.v);
    ray.Direction = normalize(focalPoint - ray.Origin);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;
}

float3 PrimaryRayGen(void) {
    uint2 launchIdx = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    uint randSeed = InitRand(launchIdx.x + launchIdx.y * launchDim.x, gSceneCB.frameCount);

    float2 pixel = (launchIdx + float2(0.5f, 0.5f)) / launchDim;;
    if (gSettingsCB.enableJitterCamera) {
        pixel += (gCameraCB.jitter / launchDim);
    }

    RayDesc ray;
    if (gSettingsCB.enableLensCamera) {
        LensCameraRay(pixel, randSeed, ray);
    } else {
        PinholdCameraRay(pixel, ray);
    }

    PrimaryRayPayload payload = { float3(0, 0, 0), randSeed, 1 };

    TraceRay(gRtScene, 
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::PrimaryRay], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::PrimaryRay], 
             ray, 
             payload);

    return payload.color;
}

float3 IndirectRayGen(float3 origin, float3 direction, inout uint seed, uint depth) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = normalize(direction);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;

    IndirectRayPayload payload = { float3(0, 0, 0), seed, depth + 1 };

    TraceRay(gRtScene, 
             RAY_FLAG_NONE, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::IndirectRay], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::IndirectRay], 
             ray, 
             payload);

    return payload.color;
}

float ShadowRayGen(float3 origin, float3 direction, float tMax) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = normalize(direction);
    ray.TMin = 1e-4f;
    ray.TMax = tMax;

    ShadowRayPayload payload = { 0 };

    TraceRay(gRtScene, 
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::ShadowRay], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::ShadowRay], 
             ray, 
             payload);

    return payload.value;
}

#endif // _RAYTRACING_RAYGENER_H_