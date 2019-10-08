#ifndef _RAYTRACING_RAYTRACE_H_
#define _RAYTRACING_RAYTRACE_H_

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
             RAY_FLAG_NONE, 
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

float3 GGXDirect(inout uint randSeed, float3 V, in HitSample hs) {

    // Pick a random light from our scene to shoot a shadow ray towards
    // uint lightIdx = min(uint(gSceneCB.lightCount * NextRand(randSeed)), gSceneCB.lightCount - 1);
    uint lightIdx = 0;

    // Query the scene to find info about the randomly selected light
    LightSample ls;
    EvaluateLight(gLights[lightIdx], hs.position, ls);

    float3 L = ls.L;

    // Shoot our shadow ray to our randomly selected light
    // float shadow = float(gSceneCB.lightCount) * ShadowRayGen(hs.position, L, length(ls.position - hs.position));
    float shadow = ShadowRayGen(hs.position, L, length(ls.position - hs.position));

    float3 N = hs.normal;
    // Compute our lambertion term (N dot L)
    float NdotL = saturate(dot(N, L));

    // Compute half vectors and additional dot products for GGX
    float3 H = normalize(V + L);
    float NdotH = saturate(dot(N, H));
    float NdotV = saturate(dot(N, V));
    float HdotV = saturate(dot(H, V));

    // If it's a dia-electric (like plastic) use F0 of 0.04
    // And if it's a metal, use the albedo color as F0 (metal workflow)
    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), hs.baseColor.rgb, hs.metalic);

    // Evaluate terms for our GGX BRDF model
    float  D = GGXNormalDistribution(NdotH, hs.roughness);
    float  G = GGXSchlickMaskingTerm(NdotL, NdotV, hs.roughness);
    float3 F = SchlickFresnel(f0, HdotV);

    // Evaluate the Cook-Torrance Microfacet BRDF model
    float3 specular = D * G * F / max(0.001f, (4 * NdotV * NdotL));

    float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0 - hs.metalic);
    float3 diffuse = hs.baseColor.rgb * kD * M_1_PI;

    // Compute our final color (combining diffuse lobe plus specular GGX lobe)
    return (diffuse + specular)* NdotL * ls.diffuse * shadow;
}

float3 GGXIndirect(inout uint randSeed, float3 V, in HitSample hs, uint rayDepth) {
    float3 N = hs.normal;

    float probability = NextRand(randSeed);
    float diffuceRatio = 1.0f - hs.metalic;

    // If we randomly selected to sample our diffuse lobe...
    if (probability < diffuceRatio) {
        // Shoot a randomly selected cosine-sampled diffuse ray.
        float3 L = CosHemisphereSample(randSeed, N);
        float3 bounceColor = IndirectRayGen(hs.position, L, randSeed, rayDepth + 1);

        float NdotL = saturate(dot(N, L));
        //float pdf = NdotL * M_1_PI;

        // Accumulate the color: (NdotL * incomingLight * difs / pi) 
        // Probability of sampling:  (NdotL / pi) * probDifs
        return NdotL * bounceColor * hs.baseColor.rgb;

        // Otherwise we randomly selected to sample our GGX lobe
    } else {
        // Randomly sample the NDF to get a microfacet in our BRDF to reflect off of
        float3 L = GGXMicrofacet(randSeed, hs.roughness, N);
        float3 H = normalize(V + L);

        // Compute our color by tracing a ray in this direction
        float3 bounceColor = IndirectRayGen(hs.position, L, randSeed, rayDepth + 1);

        // Compute some dot products needed for shading
        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
        float NdotV = saturate(dot(N, V));
        //float LdotH = saturate(dot(L, H));
        float HdotV = saturate(dot(H, V));
        float VdotL = saturate(dot(V, L));

        // If it's a dia-electric (like plastic) use F0 of 0.04
        // And if it's a metal, use the albedo color as F0 (metal workflow)
        float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), hs.baseColor.rgb, hs.metalic);

        // Evaluate our BRDF using a microfacet BRDF model
        float  D = GGXNormalDistribution(NdotH, hs.roughness);             // The GGX normal distribution
        float  G = GGXSchlickMaskingTerm(NdotL, NdotV, hs.roughness);      // Use Schlick's masking term approx
        float3 F = SchlickFresnel(f0, HdotV);                              // Use Schlick's approx to Fresnel
        float3 specular = D * G * F / max(0.001f, (4 * NdotL * NdotV));    // The Cook-Torrance microfacet BRDF

        // Accumulate the color:  ggx-BRDF * incomingLight * NdotL / probability-of-sampling
        //float pdf = D * NdotL / (4 * VdotL);
        //return specular * bounceColor * NdotL / pdf;
        return specular * bounceColor * 4 * VdotL / D;
    }
}

// Lambertian material path trace
// https://en.wikipedia.org/wiki/Path_tracing#targetText=Path%20tracing%20is%20a%20computer,the%20surface%20of%20an%20object.
float3 LambertianTracePath(inout uint randSeed, in uint rayDepth, in Attributes attribs) {
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

// optimzed of function LambertianTracePath
float3 LambertianTracePathOpt(inout uint randSeed, in uint rayDepth, in Attributes attribs) {
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
    HitSample hs;
    EvaluateHit(attribs, hs);

    // float roughness = hs.specular.a * hs.specular.a;
    float3 viewDir = normalize(WorldRayOrigin() - hs.position);

    // do explicit direct lighting to a random light in the scene
    float3 directColor = GGXDirect(payload.seed, viewDir, hs);

    // do indirect lighting for global illumination
    float3 indirectColor = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < gSceneCB.sampleCount; ++i) {
        indirectColor += GGXIndirect(payload.seed, viewDir, hs, payload.depth);
    }
    indirectColor *= (1.0f / gSceneCB.sampleCount);

    payload.color = saturate(directColor + indirectColor);
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
    HitSample hs;
    EvaluateHit(attribs, hs);

    //float roughness = hs.specular.a * hs.specular.a;
    float3 viewDir = normalize(WorldRayOrigin() - hs.position);

    payload.color = GGXDirect(payload.seed, viewDir, hs);

    // There is a shadow ray in GGXDirect, so reduce one for deoth
    if (payload.depth < gSceneCB.maxRayDepth) {
        payload.color += GGXIndirect(payload.seed, viewDir, hs, payload.depth);
    }
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

#endif // _RAYTRACING_RAYTRACE_H_