#ifndef _RAYTRACING_INDIRECTRAY_H_
#define _RAYTRACING_INDIRECTRAY_H_

#include "GGXShading.hlsli"

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

    //float roughness = hs.specular.a * hs.specular.a;
    float3 viewDir = normalize(gCameraCB.position - hs.position);

    payload.color = GGXDirect(payload.seed, viewDir, hs);

    if (payload.depth < gSceneCB.maxPayDepth - 1) {
        payload.color += GGXIndirect(payload.seed, viewDir, hs, payload.depth);
    }
}

#endif // _RAYTRACING_INDIRECTRAY_H_
