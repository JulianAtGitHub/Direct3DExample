#ifndef _RAYTRACING_SHADOWRAY_H_
#define _RAYTRACING_SHADOWRAY_H_

#include "Utils.hlsli"

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
