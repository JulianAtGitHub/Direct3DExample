#ifndef _RAYTRACING_INDIRECTRAY_H_
#define _RAYTRACING_INDIRECTRAY_H_

#include "Utils.hlsli"
#include "ShadowRay.hlsli"

float3 IndirectRayGen(float3 origin, float3 direction, inout uint seed) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = normalize(direction);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;

    IndirectRayPayload payload = { float3(0, 0, 0), seed };

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

[shader("miss")]
void IndirectMiss(inout IndirectRayPayload payload) {
    payload.color = EnvironmentColor(WorldRayDirection());
}

[shader("anyhit")]
void IndirectAnyHit(inout IndirectRayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFailed(RayTraceParams::AlphaThreshold, attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void IndirectClosestHit(inout IndirectRayPayload payload, in Attributes attribs) {
    HitSample hs;
    EvaluateHit(attribs, hs);

    uint lightIdx = min(uint(gSceneCB.lightCount * NextRand(payload.seed)), gSceneCB.lightCount - 1);

    // direct color from light
    LightSample ls;
    EvaluateLight(gLights[lightIdx], hs.position, ls);

    float factor = gSceneCB.lightCount;

    // direct shadow
    factor *= ShadowRayGen(hs.position, ls.L, length(ls.position - hs.position));
    float LdotN = saturate(dot(hs.normal, ls.L));

    // Return the Lambertian shading color using the physically based Lambertian term (albedo / pi)
    float3 color = ls.diffuse * hs.diffuse.rgb;
    color *= factor * LdotN * M_1_PI;
    payload.color = color;
}

#endif // _RAYTRACING_INDIRECTRAY_H_
