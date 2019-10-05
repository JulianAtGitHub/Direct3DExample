#ifndef _RAYTRACING_SHADING_H_
#define _RAYTRACING_SHADING_H_

/**Random Functions***********************************************************/

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint InitRand(uint val0, uint val1, uint backoff = 16) {
    uint v0 = val0, v1 = val1, s0 = 0;

    [unroll]
    for (uint n = 0; n < backoff; n++) {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
inline float NextRand(inout uint s) {
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

/**Hemi-Sphere Sampler***********************************************************/
// https://blog.thomaspoulet.fr/uniform-sampling-on-unit-hemisphere/

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
inline float3 PerpendicularVector(float3 u) {
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
inline float3 CosHemisphereSample(inout uint randSeed, float3 hitNorm) {
    // Get 2 random numbers to select our sample with
    float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = PerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(randVal.x);
    float phi = 2.0f * M_PI * randVal.y;

    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

// Get a uniform weighted random vector centered around a specified normal direction.
inline float3 UniformHemisphereSample(inout uint randSeed, float3 hitNorm) {
    // Get 2 random numbers to select our sample with
    float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = PerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(max(0.0f,1.0f - randVal.x*randVal.x));
    float phi = 2.0f * M_PI * randVal.y;

    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * randVal.x;
}

/** GGX ***********************************************************/

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
    float k = roughness * roughness * 0.5;

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


/**Light Sampler***********************************************************/

inline void EvaluateDirectLight(in Light light, in float3 hitPos, inout LightSample ls) {
    ls.L = -normalize(light.direction);
    ls.diffuse = light.intensity;
    ls.specular = light.intensity;
    ls.position = hitPos + ls.L * 1e+30f;
}

inline void EvaluatePointLight(in Light light, in float3 hitPos, inout LightSample ls) {
    ls.position = light.position;

    ls.L = light.position - hitPos;
    // Avoid NaN
    float distSquared = dot(ls.L, ls.L);
    ls.L = (distSquared > 1e-5f) ? normalize(ls.L) : 0;

    // The 0.01 is to avoid infs when the light source is close to the shading point
    float falloff = 1 / (0.0001 /*0.01 * 0.01*/ + distSquared);

    ls.diffuse = light.intensity * falloff;
    ls.specular = ls.diffuse;
}

inline void EvaluateSpotLight(in Light light, in float3 hitPos, inout LightSample ls) {
    ls.position = light.position;

    ls.L = light.position - hitPos;
    // Avoid NaN
    float distSquared = dot(ls.L, ls.L);
    ls.L = (distSquared > 1e-5f) ? normalize(ls.L) : 0;

    // The 0.01 is to avoid infs when the light source is close to the shading point
    float falloff = 1 / (0.0001 /*0.01 * 0.01*/ + distSquared);

    // Calculate the falloff for spot-lights
    float cosTheta = dot(-ls.L, light.direction); // cos of angle of light orientation
    float theta = acos(cosTheta);
    if(theta > light.openAngle * 0.5) {
        falloff = 0;
    } else if(light.penumbraAngle > 0) {
        float deltaAngle = light.openAngle - theta;
        falloff *= saturate((deltaAngle - light.penumbraAngle) / light.penumbraAngle);
    }

    ls.diffuse = light.intensity * falloff;
    ls.specular = ls.diffuse;
}

inline void EvaluateLight(in Light light, in float3 hitPos, inout LightSample ls) {
    switch(light.type) {
        case RayTraceParams::DirectLight:   return EvaluateDirectLight(light, hitPos, ls);
        case RayTraceParams::PointLight:    return EvaluatePointLight(light, hitPos, ls);
        case RayTraceParams::SpotLight:     return EvaluateSpotLight(light, hitPos, ls);
        default:                            return;
    }
}

/**Others Utils***********************************************************/

inline uint3 HitTriangle(void) {
    uint indexOffset = gGeometries[InstanceID()].indexOffset + PrimitiveIndex() * 3; // indicesPerTriangle(3)
    return gIndices.Load3(indexOffset * 4); // indexSizeInBytes(4)
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
inline float2 BarycentricLerpFloat2(in float2 v0, in float2 v1, in float2 v2, in float2 bc) {
    return v0 + (v1 - v0) * bc.x + (v2 - v0) * bc.y;
}

inline float3 BarycentricLerpFloat3(in float3 v0, in float3 v1, in float3 v2, in float2 bc) {
    return v0 + (v1 - v0) * bc.x + (v2 - v0) * bc.y;
}

inline void EvaluateHit(in Attributes attribs, inout HitSample hs) {
    uint3 idx = HitTriangle();
    Geometry geo = gGeometries[InstanceID()];

    Vertex vert0 = gVertices[idx.x];
    Vertex vert1 = gVertices[idx.y];
    Vertex vert2 = gVertices[idx.z];

    // position
    hs.position = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    // texcoord
    float2 hitTexCoord = BarycentricLerpFloat2(vert0.texCoord, vert1.texCoord, vert2.texCoord, attribs.barycentrics);

    // normal
    float3 hitNormal = BarycentricLerpFloat3(vert0.normal, vert1.normal, vert2.normal, attribs.barycentrics);
    hitNormal = mul((float3x3)ObjectToWorld3x4(), hitNormal);

    if (geo.texInfo.w != ~0) {
        float3 hitTangent = BarycentricLerpFloat3(vert0.tangent, vert1.tangent, vert2.tangent, attribs.barycentrics);
        hitTangent = mul((float3x3)ObjectToWorld3x4(), hitTangent);

        float3 hitBitangent = BarycentricLerpFloat3(vert0.bitangent, vert1.bitangent, vert2.bitangent, attribs.barycentrics);
        hitBitangent = mul((float3x3)ObjectToWorld3x4(), hitBitangent);

        float3x3 TBN = float3x3(normalize(hitTangent), normalize(hitBitangent), normalize(hitNormal));
        TBN = transpose(TBN);

        float3 normal = gMatTextures[geo.texInfo.w].SampleLevel(gSampler, hitTexCoord, 0).rgb;
        normal = normalize(normal * 2.0f - 1.0f);
        normal = mul(TBN, normal);
        hs.normal = normalize(normal);
    } else {
        hs.normal = normalize(hitNormal);
    }

    hs.baseColor = gMatTextures[geo.texInfo.x].SampleLevel(gSampler, hitTexCoord, 0);
    hs.metalic = gMatTextures[geo.texInfo.y].SampleLevel(gSampler, hitTexCoord, 0).r;
    hs.roughness = gMatTextures[geo.texInfo.z].SampleLevel(gSampler, hitTexCoord, 0).r;
    hs.roughness = max(0.08, hs.roughness);
}


inline bool AlphaTestFailed(float threshold, Attributes attribs) {
    uint3 idx = HitTriangle();
    Geometry geo = gGeometries[InstanceID()];
    if (geo.isOpacity) {
        return false;
    }

    float2 hitTexCoord = BarycentricLerpFloat2(gVertices[idx.x].texCoord, gVertices[idx.y].texCoord, gVertices[idx.z].texCoord, attribs.barycentrics);
    float4 baseColor = gMatTextures[geo.texInfo.x].SampleLevel(gSampler, hitTexCoord, 0);

    if (baseColor.a < threshold) {
        return true;
    }
    return false;
}

// Returns a relative luminance of an input linear RGB color in the ITU-R BT.709 color space
//  param RGBColor linear HDR RGB color in the ITU-R BT.709 color space
inline float Luminance(float3 rgb) {
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

// Convert our world space direction to a (u,v) coord in a latitude-longitude spherical map
inline float2 DirToLatLong(float3 dir) {
    float3 p = normalize(dir);
    float u = (1.f + atan2(p.x, -p.z) * M_1_PI) * 0.5f; // atan2 => [-PI, PI]
    float v = acos(p.y) * M_1_PI; //  acos => [0, PI]
    return float2(u, 1.0 - v);
}

inline float3 EnvironmentColor(float3 dir) {
    if (gSettingsCB.enableEnvironmentMap) {
        float2 uv = DirToLatLong(dir);
        return gEnvTexture.SampleLevel(gSampler, uv, 0).rgb;
    } else {
        return gSceneCB.bgColor.rgb;
    }
}

#endif // _RAYTRACING_SHADING_H_
