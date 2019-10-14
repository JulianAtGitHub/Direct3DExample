#ifndef _PATHTRACE_PATHTRACE_H_
#define _PATHTRACE_PATHTRACE_H_

#include "Shading.hlsli"

inline void PinholdCameraRay(in float2 pixel, out RayDesc ray) {
    float2 ndc = float2(2, -2) * pixel + float2(-1, 1);
    ray.Origin = gCameraCB.position.xyz;
    ray.Direction = normalize(ndc.x * gCameraCB.u.xyz + ndc.y * gCameraCB.v.xyz + gCameraCB.w.xyz);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;
}

inline void LensCameraRay(in float2 pixel, inout uint randSeed, out RayDesc ray) {
    float2 ndc = float2(2, -2) * pixel + float2(-1, 1);

    float3 rayDir = ndc.x * gCameraCB.u.xyz + ndc.y * gCameraCB.v.xyz + gCameraCB.w.xyz;
    rayDir /= length(gCameraCB.w);

    float3 focalPoint = gCameraCB.position.xyz + gCameraCB.focalLength * rayDir;

    // Get point on lens (in polar coords then convert to Cartesian)
    float2 rnd = float2(2.0f * M_PI * NextRand(randSeed),  gCameraCB.lensRadius * NextRand(randSeed));
    float2 uv = float2(cos(rnd.x) * rnd.y, sin(rnd.x) * rnd.y);

    ray.Origin = gCameraCB.position.xyz + uv.x * normalize(gCameraCB.u.xyz) + uv.y * normalize(gCameraCB.v.xyz);
    ray.Direction = normalize(focalPoint - ray.Origin);
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;
}

float3 PrimaryRayGen(void) {
    uint2 launchIdx = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    uint randSeed = InitRand(launchIdx.x + launchIdx.y * launchDim.x, gSceneCB.frameSeed);

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

    PrimaryRayPayload payload = { float3(0, 0, 0), randSeed, 0 };

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

    IndirectRayPayload payload = { float3(0, 0, 0), seed, depth };

    TraceRay(gRtScene, 
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 
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

// Lambertian material path trace
// https://en.wikipedia.org/wiki/Path_tracing#targetText=Path%20tracing%20is%20a%20computer,the%20surface%20of%20an%20object.
float3 LambertianScatter(inout uint randSeed, in uint rayDepth, in Attributes attribs) {
    if (rayDepth >= gSceneCB.maxRayDepth) {
        return float3(0.0f, 0.0f, 0.0f);
    }

    HitSample hs;
    EvaluateHit(attribs, hs);

    float3 reflectance = hs.baseColor.rgb;
    float3 emittance = hs.emissive.rgb;

    float3 N = hs.normal;
    float3 L = UniformHemisphereSample(randSeed, N);
    float NDotL = saturate(dot(N, L));

    const float pdf = 1.0f / (2.0f * M_PI);
    
    float3 BRDF = reflectance / M_PI;

    float3 incoming = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < gSceneCB.sampleCount; ++i) {
        incoming += IndirectRayGen(hs.position, L, randSeed, rayDepth + 1);
    }
    incoming /= gSceneCB.sampleCount;

    float3 color = emittance + (BRDF * incoming * NDotL / pdf);

    return color;
}

// optimzed of function LambertianScatter
float3 LambertianScatterOpt(inout uint randSeed, in uint rayDepth, in Attributes attribs) {
    if (rayDepth >= gSceneCB.maxRayDepth) {
        return float3(0.0f, 0.0f, 0.0f);
    }

    HitSample hs;
    EvaluateHit(attribs, hs);

    float3 reflectance = hs.baseColor.rgb;

    float3 N = hs.normal;
    float3 L = UniformHemisphereSample(randSeed, N);
    float NDotL = saturate(dot(N, L));

    float3 incoming = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < gSceneCB.sampleCount; ++i) {
        incoming += IndirectRayGen(hs.position, L, randSeed, rayDepth + 1);
    }
    incoming /= gSceneCB.sampleCount;

    return hs.emissive.rgb + reflectance * incoming * NDotL * 2.0f;
}

/**Primary Ray***********************************************************/

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
    payload.color += LambertianScatterOpt(payload.seed, payload.depth, attribs);
}

/**Indirect Ray***********************************************************/

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
    payload.color += LambertianScatterOpt(payload.seed, payload.depth, attribs);
}

/**Shadow Ray***********************************************************/

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

#endif // _PATHTRACE_PATHTRACE_H_