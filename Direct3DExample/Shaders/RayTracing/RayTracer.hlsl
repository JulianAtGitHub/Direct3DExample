
#include "Common.hlsli"
#include "Utils.hlsli"
#include "IndirectRay.hlsli"
#include "ShadowRay.hlsli"

inline void WriteToOutput(in PrimaryRayPayload payload, in uint2 index) {
    if (gSettingsCB.enableAccumulate) {
        float4 lastFrameColor = gRenderTarget[index];
        float3 finalColor = (lastFrameColor.rgb * gSceneCB.accumCount + payload.color) / (gSceneCB.accumCount + 1);
        gRenderTarget[index] = float4(finalColor, 1);
        gRenderDisplay[index] = float4(finalColor, 1);
    } else {
        gRenderTarget[index] = float4(payload.color, 1);
        gRenderDisplay[index] = float4(payload.color, 1);
    }
}

inline void PinholdCameraRay(in float2 pixel, out RayDesc ray) {
    float2 ndc = float2(2, -2) * pixel + float2(-1, 1);                    
    ray.Origin = gCameraCB.pos;
    ray.Direction = normalize(ndc.x * gCameraCB.u + ndc.y * gCameraCB.v + gCameraCB.w);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;
}

inline void LensCameraRay(in float2 pixel, inout uint randSeed, out RayDesc ray) {
    float2 ndc = float2(2, -2) * pixel + float2(-1, 1);

    float3 rayDir = ndc.x * gCameraCB.u + ndc.y * gCameraCB.v + gCameraCB.w;
    rayDir /= length(gCameraCB.w);

    float3 focalPoint = gCameraCB.pos + gCameraCB.focalLength * rayDir;

    // Get point on lens (in polar coords then convert to Cartesian)
    float2 rnd = float2(2.0f * M_PI * NextRand(randSeed),  gCameraCB.lensRadius * NextRand(randSeed));
    float2 uv = float2(cos(rnd.x) * rnd.y, sin(rnd.x) * rnd.y);

    ray.Origin = gCameraCB.pos + uv.x * normalize(gCameraCB.u) + uv.y * normalize(gCameraCB.v);
    ray.Direction = normalize(focalPoint - ray.Origin);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;
}

[shader("raygeneration")]
void RayGener() {
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

    PrimaryRayPayload payload = { float3(0, 0, 0), randSeed };

    TraceRay(gRtScene, 
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::PrimaryRay], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::PrimaryRay], 
             ray, 
             payload);

    WriteToOutput(payload, launchIdx);
}

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

    // Random select a light 
    uint lightIdx = min(uint(gSceneCB.lightCount * NextRand(payload.seed)), gSceneCB.lightCount - 1);

    // direct color from light
    LightSample ls;
    EvaluateLight(gLights[lightIdx], hs.position, ls);

    float factor = gSceneCB.lightCount;

    // direct shadow
    factor *= ShadowRayGen(hs.position, ls.L, length(ls.position - hs.position));
    float LdotN = saturate(dot(hs.normal, ls.L));

    // Modulate based on the physically based Lambertian term (albedo/pi)
    float3 shadeColor = ls.diffuse * hs.diffuse.rgb; 
    shadeColor *= factor * LdotN * M_1_PI;

    // Indirect light
    if (gSettingsCB.enableIndirectLight) {
        float3 indirectDir = CosHemisphereSample(payload.seed, hs.normal);
        float3 indirectColor = IndirectRayGen(hs.position, indirectDir, payload.seed);

        // Get NdotL for our selected ray direction
        float NdotL = saturate(dot(hs.position, indirectDir));
        shadeColor += indirectColor * hs.diffuse.rgb;
    }

    payload.color = shadeColor;
}

