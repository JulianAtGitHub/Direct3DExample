#ifndef _RAYTRACING_AORAY_H_
#define _RAYTRACING_AORAY_H_

#include "Utils.hlsli"

float AORayGen(float3 hitPos, float3 hitNorm, inout uint seed) {
    float3 rayDir = CosHemisphereSample(seed, hitNorm);

    RayDesc ray;
    ray.Origin = hitPos;
    ray.Direction = normalize(rayDir);
    ray.TMin = 1e-4f;
    ray.TMax = gSceneCB.aoRadius;

    AORayPayload payload = { 0 };

    TraceRay(gRtScene, 
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::AORay], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::AORay], 
             ray, 
             payload);

    return payload.value;
}

[shader("miss")]
void AOMiss(inout AORayPayload payload) {
    payload.value = 1;
}

[shader("anyhit")]
void AOAnyHit(inout AORayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFailed(RayTraceParams::AlphaThreshold, attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void AOClosestHit(inout AORayPayload payload, in Attributes attribs) {
    // nothing to do here
}

#endif // _RAYTRACING_AORAY_H_
