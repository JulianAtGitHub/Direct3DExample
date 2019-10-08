#ifndef _PATHTRACE_SHADING_H_
#define _PATHTRACE_SHADING_H_

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

    if (geo.texInfo.w != TEX_INDEX_INVALID) {
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

    hs.baseColor = geo.texInfo.x != TEX_INDEX_INVALID ? gMatTextures[geo.texInfo.x].SampleLevel(gSampler, hitTexCoord, 0) : geo.diffuseColor;
    hs.emissive = geo.emissiveColor;
}


inline bool AlphaTestFailed(float threshold, Attributes attribs) {
    uint3 idx = HitTriangle();
    Geometry geo = gGeometries[InstanceID()];
    if (geo.isOpacity || geo.texInfo.x == TEX_INDEX_INVALID) {
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

#endif // _PATHTRACE_SHADING_H_
