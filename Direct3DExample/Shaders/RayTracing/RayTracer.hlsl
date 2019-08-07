
#include "Common.hlsli"
#include "Utils.hlsli"
#include "AORay.hlsli"
#include "ShadowRay.hlsli"

inline void WriteToOutput(in PrimaryRayPayload payload, in uint2 index) {
    if (gSettingsCB.enableAccumulate) {
        float4 lastFrameColor = gRenderTarget[index];
        float4 finalColor = (lastFrameColor * gSceneCB.accumCount + payload.color) / (gSceneCB.accumCount + 1);
        gRenderTarget[index] = finalColor;
        gRenderDisplay[index] = finalColor;
    } else {
        gRenderTarget[index] = payload.color;
        gRenderDisplay[index] = payload.color;
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

    PrimaryRayPayload payload = { float4(0, 0, 0, 1), randSeed };

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
    float2 dims;
    gEnvTexture.GetDimensions(dims.x, dims.y);
    float2 uv = ToLatLong( WorldRayDirection() );
    payload.color = gEnvTexture.SampleLevel(gSampler, uv, 0);
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
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    uint3 idx = HitTriangle();
    Geometry geo = gGeometries[InstanceID()];

    // position
    float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    //float4 wsPosition = float4(hitPosition, 1.0f);
    //gOutputs[RayTraceParams::WsPosition][launchIndex] = wsPosition;

    // normal
    float3 normals[3] = { gVertices[idx.x].normal, gVertices[idx.y].normal, gVertices[idx.z].normal };
    float3 hitNormal = LerpFloat3Attributes(normals, attribs);
    hitNormal = normalize(mul((float3x3) ObjectToWorld3x4(), hitNormal));
    //float4 wsNormal = float4(hitNormal, length(hitPosition - gCameraCB.pos));
    //gOutputs[RayTraceParams::WsNormal][launchIndex] = wsNormal;

    float2 texCoords[3] = { gVertices[idx.x].texCoord, gVertices[idx.y].texCoord, gVertices[idx.z].texCoord };
    float2 hitTexCoord = LerpFloat2Attributes(texCoords, attribs);

    /**
    BaseColor
        - RGB - Base Color
        - A   - Transparency
    Specular
        - R - Occlusion
        - G - Metalness
        - B - Roughness
        - A - Reserved
    Emissive
        - RGB - Emissive Color
        - A   - Unused
    */
    float4 baseColor = gMatTextures[geo.texInfo.x].SampleLevel(gSampler, hitTexCoord, 0);
    float4 spec = gMatTextures[geo.texInfo.y].SampleLevel(gSampler, hitTexCoord, 0);

    // diffuse 
    float4 diffColor = float4(lerp(baseColor.rgb, float3(0, 0, 0), spec.b), baseColor.a);
    //gOutputs[RayTraceParams::MatDiffuse][launchIndex] = diffColor;

    // specular UE4 uses 0.08 multiplied by a default specular value of 0.5 as a base, hence the 0.04
    // Clamp the roughness so that the BRDF won't explode
    float4 specColor = float4(lerp(float3(0.04f, 0.04f, 0.04f), baseColor.rgb, spec.b), max(0.08, spec.g));
    //gOutputs[RayTraceParams::MatSpecular][launchIndex] = specColor;

    // other mats
    //gOutputs[RayTraceParams::MatEmissive][launchIndex] = float4(0, 0, 0, 0);
    //gOutputs[RayTraceParams::MatExtra][launchIndex] = float4(1, 0, 0, 0);

    float3 dirColor = float3(0.0, 0.0, 0.0);
    // Iterate over the lights
    for (int i = 0; i < gSceneCB.lightCount; ++ i) {
        LightSample ls;
        EvaluateLight(gLights[i], hitPosition, ls);

        float shadowFactor = ShadowRayGen(hitPosition, ls.L, length(ls.position - hitPosition));

        float LdotN = saturate(dot(hitNormal, ls.L));
        dirColor += shadowFactor * LdotN * ls.diffuse; 
    }

    // Modulate based on the physically based Lambertian term (albedo/pi)
    dirColor *= diffColor.rgb / M_PI;

    payload.color = float4(dirColor, 1);

    // calculate AO
    // uint randSeed = payload.randSeed;
    // float ambientOcclusion = AORayGen(hitPosition, hitNormal, randSeed);
}

