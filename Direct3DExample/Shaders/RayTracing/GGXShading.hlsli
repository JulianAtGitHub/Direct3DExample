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
    float d = ((NdotH * a2 - NdotH) * NdotH + 1);
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
    float g_v = NdotV / (NdotV*(1 - k) + k);
    float g_l = NdotL / (NdotL*(1 - k) + k);

    // Return G(v) * G(l)
    return g_v * g_l;
}

// Traditional Schlick approximation to the Fresnel term (also from Schlick 1994)
//
// This function can be used for "F" in the Cook-Torrance model:  D*G*F / (4*NdotL*NdotV)
inline float3 SchlickFresnel(float3 f0, float u) {
    return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - u, 5.0f);
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
    float cosThetaH = sqrt(max(0.0f, (1.0 - randVal.x) / ((a2 - 1.0) * randVal.x + 1)));
    float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
    float phiH = randVal.y * M_PI * 2.0f;

    // Get our GGX NDF sample (i.e., the half vector)
    return T * (sinThetaH * cos(phiH)) + B * (sinThetaH * sin(phiH)) + hitNorm * cosThetaH;
}

float3 GGXDirect(inout uint randSeed, float3 hit, float3 N, float3 V, float3 difs, float3 spec, float rough) {
    // Pick a random light from our scene to shoot a shadow ray towards
    uint lightIdx = min(uint(gSceneCB.lightCount * NextRand(randSeed)), gSceneCB.lightCount - 1);

    // Query the scene to find info about the randomly selected light
    LightSample ls;
    EvaluateLight(gLights[lightIdx], hit, ls);
    float3 L = ls.L;

    // Compute our lambertion term (N dot L)
    float NdotL = saturate(dot(N, L));

    // Shoot our shadow ray to our randomly selected light
    float shadowFactor = float(gSceneCB.lightCount) * ShadowRayGen(hit, L, length(ls.position - hit));

    // Compute half vectors and additional dot products for GGX
    float3 H = normalize(V + L);
    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));
    float NdotV = saturate(dot(N, V));

    // Evaluate terms for our GGX BRDF model
    float  D = GGXNormalDistribution(NdotH, rough);
    float  G = GGXSchlickMaskingTerm(NdotL, NdotV, rough);
    float3 F = SchlickFresnel(spec, LdotH);

    // Evaluate the Cook-Torrance Microfacet BRDF model
    //     Cancel out NdotL here & the next eq. to avoid catastrophic numerical precision issues.
    //float3 ggxFactor = D * G * F / (4 * NdotV * NdotL);
    float3 ggxFactor = D * G * F / (4 * NdotV);

    // Compute our final color (combining diffuse lobe plus specular GGX lobe)
    //return shadowFactor * ls.diffuse * (NdotL * ggxFactor + NdotL * difs / M_PI);
    return shadowFactor * ls.diffuse * (ggxFactor + NdotL * difs / M_PI);
}

// Our material has have both a diffuse and a specular lobe.  
//     With what probability should we sample the diffuse one?
inline float ProbabilityToSampleDiffuse(float3 difs, float3 spec) {
    float lumDifs = max(0.01f, Luminance(difs));
    float lumSpec = max(0.01f, Luminance(spec));
    return lumDifs / (lumDifs + lumSpec);
}

float3 GGXIndirect(inout uint randSeed, float3 hit, float3 N, float3 V, float3 difs, float3 spec, float rough, uint rayDepth) {
    // We have to decide whether we sample our diffuse or specular/ggx lobe.
    float probDifs = ProbabilityToSampleDiffuse(difs, spec);
    bool chooseDifs = (NextRand(randSeed) < probDifs);

    // We'll need NdotV for both diffuse and specular...
    float NdotV = saturate(dot(N, V));

    // If we randomly selected to sample our diffuse lobe...
    if (chooseDifs) {
        // Shoot a randomly selected cosine-sampled diffuse ray.
        float3 L = CosHemisphereSample(randSeed, N);
        float3 bounceColor = IndirectRayGen(hit, L, randSeed, rayDepth);

        // Accumulate the color: (NdotL * incomingLight * difs / pi) 
        // Probability of sampling:  (NdotL / pi) * probDifs
        return bounceColor * difs / probDifs;

    // Otherwise we randomly selected to sample our GGX lobe
    } else {
        // Randomly sample the NDF to get a microfacet in our BRDF to reflect off of
        float3 H = GGXMicrofacet(randSeed, rough, N);

        // Compute the outgoing direction based on this (perfectly reflective) microfacet
        float3 L = normalize(2.f * dot(V, H) * H - V);

        // Compute our color by tracing a ray in this direction
        float3 bounceColor = IndirectRayGen(hit, L, randSeed, rayDepth);

        // Compute some dot products needed for shading
        float  NdotL = saturate(dot(N, L));
        float  NdotH = saturate(dot(N, H));
        float  LdotH = saturate(dot(L, H));

        // Evaluate our BRDF using a microfacet BRDF model
        float  D = GGXNormalDistribution(NdotH, rough);          // The GGX normal distribution
        float  G = GGXSchlickMaskingTerm(NdotL, NdotV, rough);   // Use Schlick's masking term approx
        float3 F = SchlickFresnel(spec, LdotH);                  // Use Schlick's approx to Fresnel
        float3 ggxFactor = D * G * F / (4 * NdotL * NdotV);        // The Cook-Torrance microfacet BRDF

        // What's the probability of sampling vector H from getGGXMicrofacet()?
        float  ggxProb = D * NdotH / (4 * LdotH);

        // Accumulate the color:  ggx-BRDF * incomingLight * NdotL / probability-of-sampling
        //    -> Should really simplify the math above.
        return NdotL * bounceColor * ggxFactor / (ggxProb * (1.0f - probDifs));
    }
}

#endif // _RAYTRACING_GGXSHADING_H_