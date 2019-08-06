#ifndef _RAYTRACING_SHADOWRAY_H_
#define _RAYTRACING_SHADOWRAY_H_

#include "Utils.hlsli"

float ShadowRayGen(float3 hitPos, float3 toLight, float tMax) {
    RayDesc ray;
    ray.Origin = hitPos;
    ray.Direction = normalize(toLight);
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

[shader("miss")]
void ShadowMiss(inout ShadowRayPayload payload) {
    payload.value = 1;
}

[shader("anyhit")]
void ShadowAnyHit(inout ShadowRayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFailed(RayTraceParams::AlphaThreshold, attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void ShadowClosestHit(inout ShadowRayPayload payload, in Attributes attribs) {
    // nothing to do here
}

#endif // _RAYTRACING_SHADOWRAY_H_
