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

    float roughness = hs.specular.a * hs.specular.a;
    float3 viewDir = normalize(gCameraCB.position - hs.position);

    payload.color = GGXDirect(payload.seed, hs.position, hs.normal, viewDir, hs.diffuse.rgb, hs.specular.rgb, roughness);

    // Do indirect illumination at this hit location (if we haven't traversed too far)
    if (payload.depth < gSceneCB.maxPayDepth - 1) {
        // Use the same normal for the normal-mapped and non-normal mapped vectors... This means we could get light
        //     leaks at secondary surfaces with normal maps due to indirect rays going below the surface.  This
        //     isn't a huge issue, but this is a (TODO: fix)
        payload.color += GGXIndirect(payload.seed, hs.position, hs.normal, viewDir, hs.diffuse.rgb, hs.specular.rgb, roughness, payload.depth);
    }
}

#endif // _RAYTRACING_INDIRECTRAY_H_
