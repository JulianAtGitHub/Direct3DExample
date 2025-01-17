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
             INSTANCE_MASK, 
             HIT_GROUP_INDEX[PrimaryRay], 
             0, 
             MISS_INDEX[PrimaryRay], 
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
             INSTANCE_MASK, 
             HIT_GROUP_INDEX[IndirectRay], 
             0, 
             MISS_INDEX[IndirectRay], 
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
             INSTANCE_MASK, 
             HIT_GROUP_INDEX[ShadowRay], 
             0, 
             MISS_INDEX[ShadowRay], 
             ray, 
             payload);

    return payload.value;
}

inline float3 LambertianScatter(inout uint randSeed, in uint rayDepth, inout HitSample hs) {
    float3 N = hs.normal;
    float3 L = UniformHemisphereSample(randSeed, N);
    float3 intensity = hs.albedo.rgb * IndirectRayGen(hs.position, L, randSeed, rayDepth + 1);

    return hs.emissive + intensity;
}

inline float3 MetalScatter(inout uint randSeed, in uint rayDepth, inout HitSample hs) {
    float3 N = hs.normal;
    float3 R = reflect(WorldRayDirection(), N);
    float3 L = normalize(R + UniformHemisphereSample(randSeed, N) * hs.roughness);
    float NDotL = dot(N, L);
    float3 intensity = float3(0.0f, 0.0f, 0.0f);
    if (NDotL > 0.0f) {
        intensity = hs.albedo.rgb * IndirectRayGen(hs.position, L, randSeed, rayDepth + 1);
    }

    return hs.emissive + intensity;
}

inline float Schlick(float cosine, float refIdx) {
    float r0 = (1.0f - refIdx) / (1.0f + refIdx);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * pow((1.0f - cosine), 5.0f);
}

inline float3 DielectricScatter(inout uint randSeed, in uint rayDepth, inout HitSample hs) {
    float3 outwardNormal;
    float refractivity;
    float cosine;
    float3 N = hs.normal;
    float IDotN = dot(WorldRayDirection(), N);
    if (IDotN > 0.0f) {
        outwardNormal = -N;
        refractivity = hs.refractivity;
        cosine = refractivity * IDotN;
    } else {
        outwardNormal = N;
        refractivity = 1.0f / hs.refractivity;
        cosine = -IDotN;
    }

    float3 refracted = refract(WorldRayDirection(), outwardNormal, refractivity);

    float reflectPercentage;
    if (dot(refracted, refracted) < 0.0001f) {
        reflectPercentage = 1.0f;
    } else {
        reflectPercentage = Schlick(cosine, hs.refractivity);
    }

    float3 intensity = float3(0.0f, 0.0f, 0.0f);
    if (NextRand(randSeed) < reflectPercentage) {
        float3 reflected = reflect(WorldRayDirection(), N);
        intensity = hs.albedo.rgb * IndirectRayGen(hs.position, reflected, randSeed, rayDepth + 1);
    } else {
        intensity = hs.albedo.rgb * IndirectRayGen(hs.position, refracted, randSeed, rayDepth + 1);
    }

    return hs.emissive + intensity;
}

inline float3 ScatterRay(inout uint randSeed, in uint rayDepth, inout HitSample hs) {
    switch(hs.matType) {
        case LambertianMat: return LambertianScatter(randSeed, rayDepth, hs);
        case MetalMat:      return MetalScatter(randSeed, rayDepth, hs);
        case DielectricMat: return DielectricScatter(randSeed, rayDepth, hs);
        default: return float3(0.0f, 0.0f, 0.0f);
    }
}

/**Primary Ray***********************************************************/

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload) {
    payload.color = EnvironmentColor(WorldRayDirection());
}

[shader("anyhit")]
void PrimaryAnyHit(inout PrimaryRayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFailed(ALPHA_THRESHOLD, attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void PrimaryClosestHit(inout PrimaryRayPayload payload, in Attributes attribs) {
    if (payload.depth >= gSceneCB.maxRayDepth) {
        return;
    }

    HitSample hs;
    EvaluateHit(attribs, hs);

    float3 incoming = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < gSceneCB.sampleCount; ++i) {
        incoming += ScatterRay(payload.seed, payload.depth, hs);
    }
    incoming /= gSceneCB.sampleCount;

    payload.color += incoming;
}

/**Indirect Ray***********************************************************/

[shader("miss")]
void IndirectMiss(inout IndirectRayPayload payload) {
    payload.color = EnvironmentColor(WorldRayDirection());
}

[shader("anyhit")]
void IndirectAnyHit(inout IndirectRayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFailed(ALPHA_THRESHOLD, attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void IndirectClosestHit(inout IndirectRayPayload payload, in Attributes attribs) {
    if (payload.depth >= gSceneCB.maxRayDepth) {
        return;
    }

    HitSample hs;
    EvaluateHit(attribs, hs);

    float3 incoming = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < gSceneCB.sampleCount; ++i) {
        incoming += ScatterRay(payload.seed, payload.depth, hs);
    }
    incoming /= gSceneCB.sampleCount;

    payload.color += incoming;
}

/**Shadow Ray***********************************************************/

[shader("miss")]
void ShadowMiss(inout ShadowRayPayload payload) {
    payload.value = 1;
}

[shader("anyhit")]
void ShadowAnyHit(inout ShadowRayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFailed(ALPHA_THRESHOLD, attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void ShadowClosestHit(inout ShadowRayPayload payload, in Attributes attribs) {
    // nothing to do here
}

#endif // _PATHTRACE_PATHTRACE_H_