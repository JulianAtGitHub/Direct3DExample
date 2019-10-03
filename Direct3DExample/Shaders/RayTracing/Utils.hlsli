#ifndef _RAYTRACING_UTILS_H_
#define _RAYTRACING_UTILS_H_

#include "Common.hlsli"

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
float3 CosHemisphereSample(inout uint randSeed, float3 hitNorm) {
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
float3 UniformHemisphereSample(inout uint randSeed, float3 hitNorm) {
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

void EvaluateDirectLight(in Light light, in float3 hitPos, inout LightSample ls) {
    ls.L = -normalize(light.direction);
    ls.diffuse = light.intensity;
    ls.specular = light.intensity;
    ls.position = hitPos + ls.L * 1e+30f;
}

void EvaluatePointLight(in Light light, in float3 hitPos, inout LightSample ls) {
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

void EvaluateSpotLight(in Light light, in float3 hitPos, inout LightSample ls) {
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

void EvaluateLight(in Light light, in float3 hitPos, inout LightSample ls) {
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
inline float3 LerpFloat3Attributes(float3 vertexAttributes[3], Attributes attr) {
    return  vertexAttributes[0] +
            attr.barycentrics.x * (vertexAttributes[1] - vertexAttributes[0]) +
            attr.barycentrics.y * (vertexAttributes[2] - vertexAttributes[0]);
}

inline float2 LerpFloat2Attributes(float2 vertexAttributes[3], Attributes attr) {
    return  vertexAttributes[0] +
            attr.barycentrics.x * (vertexAttributes[1] - vertexAttributes[0]) +
            attr.barycentrics.y * (vertexAttributes[2] - vertexAttributes[0]);
}

inline bool AlphaTestFailed(float threshold, Attributes attribs) {
    uint3 idx = HitTriangle();
    Geometry geo = gGeometries[InstanceID()];
    if (geo.isOpacity) {
        return false;
    }
    
    float2 texCoords[3] = { gVertices[idx.x].texCoord, gVertices[idx.y].texCoord, gVertices[idx.z].texCoord };
    float2 hitTexCoord = LerpFloat2Attributes(texCoords, attribs);
    float4 baseColor = gMatTextures[geo.texInfo.x].SampleLevel(gSampler, hitTexCoord, 0);

    if (baseColor.a < threshold) {
        return true;
    }
    return false;
}

// Convert our world space direction to a (u,v) coord in a latitude-longitude spherical map
inline float2 DirToLatLong(float3 dir) {
    float3 p = normalize(dir);
    float u = (1.f + atan2(p.x, -p.z) * M_1_PI) * 0.5f; // atan2 => [-PI, PI]
    float v = acos(p.y) * M_1_PI; //  acos => [0, PI]
    return float2(u, 1.0 - v);
}

inline float3 HDRToneMapping(float3 color) {
    return color / (color + 1.0f);
}

inline float3 GammaCorrect(float3 color) {
    return pow(color, 1.0f / 2.2f);
}

// Returns a relative luminance of an input linear RGB color in the ITU-R BT.709 color space
//  param RGBColor linear HDR RGB color in the ITU-R BT.709 color space
inline float Luminance(float3 rgb) {
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

void EvaluateHit(in Attributes attribs, inout HitSample hs) {
    uint3 idx = HitTriangle();
    Geometry geo = gGeometries[InstanceID()];

    // position
    hs.position = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    // texcoord
    float2 texCoords[3] = { gVertices[idx.x].texCoord, gVertices[idx.y].texCoord, gVertices[idx.z].texCoord };
    float2 hitTexCoord = LerpFloat2Attributes(texCoords, attribs);

    // normal
    float3 normals[3] = { gVertices[idx.x].normal, gVertices[idx.y].normal, gVertices[idx.z].normal };
    float3 hitNormal = LerpFloat3Attributes(normals, attribs);
    hitNormal = mul((float3x3)ObjectToWorld3x4(), hitNormal);

    if (geo.texInfo.w != ~0) {
        float3 tangents[3] = { gVertices[idx.x].tangent, gVertices[idx.y].tangent, gVertices[idx.z].tangent };
        float3 hitTangent = LerpFloat3Attributes(tangents, attribs);
        hitTangent = mul((float3x3)ObjectToWorld3x4(), hitTangent);

        float3 bitangents[3] = { gVertices[idx.x].bitangent, gVertices[idx.y].bitangent, gVertices[idx.z].bitangent };
        float3 hitBitangent = LerpFloat3Attributes(bitangents, attribs);
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

float3 EnvironmentColor(float3 dir) {
    if (gSettingsCB.enableEnvironmentMap) {
        float2 uv = DirToLatLong(dir);
        return gEnvTexture.SampleLevel(gSampler, uv, 0).rgb;
    } else {
        return gSceneCB.bgColor.rgb;
    }
}

#endif // _RAYTRACING_UTILS_H_
