#ifndef _RAYTRACING_GGXSHADING_H_
#define _RAYTRACING_GGXSHADING_H_

#include "Utils.hlsli"
#include "RayGener.hlsli"

// The NDF for GGX, see Eqn 19 from 
//    http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf
//
// This function can be used for "D" in the Cook-Torrance model:  D*G*F / (4*NdotL*NdotV)
inline float GGXNormalDistribution(float NdotH, float roughness) {
    float a2 = roughness * roughness;
    float d = NdotH * NdotH * (a2 - 1) + 1;
    return a2 / max(0.001f, (d * d * M_PI));
}

// This from Schlick 1994, modified as per Karas in SIGGRAPH 2013 "Physically Based Shading" course
//
// This function can be used for "G" in the Cook-Torrance model:  D*G*F / (4*NdotL*NdotV)
inline float GGXSchlickMaskingTerm(float NdotL, float NdotV, float roughness) {
    // Karis notes they use alpha / 2 (or roughness^2 / 2)
    float k = roughness*roughness / 2;

    // Karis also notes they can use the following equation, but only for analytical lights
    //float k = (roughness + 1)*(roughness + 1) / 8; 

    // Compute G(v) and G(l).  These equations directly from Schlick 1994
    //     (Though note, Schlick's notation is cryptic and confusing.)
    float g_v = NdotV / (NdotV * (1 - k) + k);
    float g_l = NdotL / (NdotL * (1 - k) + k);

    // Return G(v) * G(l)
    return g_v * g_l;
}

// Traditional Schlick approximation to the Fresnel term (also from Schlick 1994)
//
// This function can be used for "F" in the Cook-Torrance model:  D*G*F / (4*NdotL*NdotV)
inline float3 SchlickFresnel(float3 f0, float cosTheta) {
    return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - cosTheta, 5.0f);
}

// Get a GGX half vector / microfacet normal, sampled according to the distribution computed by
//     the function GGXNormalDistribution() above.  
//
// When using this function to sample, the probability density is pdf = D * NdotH / (4 * HdotV)
inline float3 GGXMicrofacet(inout uint randSeed, float roughness, float3 hitNorm) {
    // Get our uniform random numbers
    float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

    // Get an orthonormal basis from the normal
    float3 B = PerpendicularVector(hitNorm);
    float3 T = cross(B, hitNorm);

    // GGX NDF sampling
    float a2 = roughness * roughness;
    float cosThetaH = sqrt(max(0.0f, (1.0 - randVal.x) / ((a2*a2 - 1.0) * randVal.x + 1)));
    float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
    float phiH = randVal.y * M_PI * 2.0f;

    // Get our GGX NDF sample (i.e., the half vector)
    return T * (sinThetaH * cos(phiH)) + B * (sinThetaH * sin(phiH)) + hitNorm * cosThetaH;
}

float3 GGXDirect(inout uint randSeed, float3 V, in HitSample hs) {
    float3 N = hs.normal;

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
    float3 diffuse = hs.baseColor.rgb * NdotL * kD * M_1_PI;

    // Compute our final color (combining diffuse lobe plus specular GGX lobe)
    return (diffuse + specular) * ls.diffuse * shadow ;
}

float3 GGXIndirect(inout uint randSeed, float3 V, in HitSample hs, uint rayDepth) {
    float3 N = hs.normal;

    float probability = NextRand(randSeed);
    float diffuceRatio = 0.5f * (1.0f - hs.metalic);

    // If we randomly selected to sample our diffuse lobe...
    if (probability < diffuceRatio) {
        // Shoot a randomly selected cosine-sampled diffuse ray.
        float3 L = CosHemisphereSample(randSeed, N);
        float3 bounceColor = IndirectRayGen(hs.position, L, randSeed, rayDepth);

        float NdotL = saturate(dot(N, L));
        //float pdf = NdotL * M_1_PI;

        // Accumulate the color: (NdotL * incomingLight * difs / pi) 
        // Probability of sampling:  (NdotL / pi) * probDifs
        return bounceColor * hs.baseColor.rgb;

    // Otherwise we randomly selected to sample our GGX lobe
    } else {
        // Randomly sample the NDF to get a microfacet in our BRDF to reflect off of
        float3 L = GGXMicrofacet(randSeed, hs.roughness, N);
        float3 H = normalize(V + L);

        // Compute our color by tracing a ray in this direction
        float3 bounceColor = IndirectRayGen(hs.position, L, randSeed, rayDepth);

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
        float  D = GGXNormalDistribution(NdotH, hs.roughness);              // The GGX normal distribution
        float  G = GGXSchlickMaskingTerm(NdotL, NdotV, hs.roughness);       // Use Schlick's masking term approx
        float3 F = SchlickFresnel(f0, HdotV);                               // Use Schlick's approx to Fresnel
        float3 specular = D * G * F / max(0.001f, (4 * NdotL * NdotV));    // The Cook-Torrance microfacet BRDF

        float pdf = D * NdotL / (4 * VdotL);

        // Accumulate the color:  ggx-BRDF * incomingLight * NdotL / probability-of-sampling
        return specular * bounceColor * NdotL / pdf;
    }
}

#endif // _RAYTRACING_GGXSHADING_H_