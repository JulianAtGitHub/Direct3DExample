#ifndef _RAYTRACING_PRIMARYRAY_H_
#define _RAYTRACING_PRIMARYRAY_H_

#include "GGXShading.hlsli"

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload) {
    payload.color = EnvironmentColor(WorldRayDirection());
}

[shader("anyhit")]
void PrimaryAnyHit(inout PrimaryRayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFailed(RayTraceParams::AlphaThreshold, attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void PrimaryClosestHit(inout PrimaryRayPayload payload, in Attributes attribs) {
    HitSample hs;
    EvaluateHit(attribs, hs);

    // float roughness = hs.specular.a * hs.specular.a;
    float3 viewDir = normalize(gCameraCB.position - hs.position);

    // do explicit direct lighting to a random light in the scene
    float3 directColor = GGXDirect(payload.seed, viewDir, hs);

    // do indirect lighting for global illumination
    float3 indirectColor = float3(0.0f, 0.0f, 0.0f);
    if (gSettingsCB.enableIndirectLight) {
        for (uint i = 0; i < gSceneCB.sampleCount; ++i) {
            indirectColor += GGXIndirect(payload.seed, viewDir, hs, payload.depth);
        }
        indirectColor *= (1.0f / gSceneCB.sampleCount);
    }

    float3 shadeColor = directColor + indirectColor;

    // Since we didn't do a good job above catching NaN's, div by 0, infs, etc.,
    //    zero out samples that would blow up our frame buffer.  Note:  You can and should
    //    do better, but the code gets more complex with all error checking conditions.
    bool colorsNan = any(isnan(shadeColor));
    if (colorsNan) {
        shadeColor = float3(0, 0, 0);
    }

    payload.color = shadeColor;
}

#endif // _RAYTRACING_PRIMARYRAY_H_