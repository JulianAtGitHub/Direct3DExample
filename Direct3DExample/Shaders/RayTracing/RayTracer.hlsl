
#include "Common.hlsli"
#include "Utils.hlsli"
#include "AORay.hlsli"

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(out float3 origin, out float3 direction) {
    float2 pixelCenter = (DispatchRaysIndex().xy + float2(0.5f, 0.5f) + gSceneCB.jitter) / DispatchRaysDimensions().xy; 
    float2 ndc = float2(2, -2) * pixelCenter + float2(-1, 1);                    
    direction = normalize(ndc.x * gSceneCB.cameraU.xyz + ndc.y * gSceneCB.cameraV.xyz + gSceneCB.cameraW.xyz);
    origin = gSceneCB.cameraPos.xyz;
}

[shader("raygeneration")]
void RayGener() {
    float3 rayPos;
    float3 rayDir;
    GenerateCameraRay(rayPos, rayDir);

    RayDesc ray;
    ray.Origin = rayPos;
    ray.Direction = rayDir;
    ray.TMin = 1e-4f;
    ray.TMax = 1e+38f;

    PrimaryRayPayload payload = { float4(0, 0, 0, 1) };

    TraceRay(gRtScene, 
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::PrimaryRay], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::PrimaryRay], 
             ray, 
             payload);

    // Write the raytraced result to the output texture.
    float4 lastFrameColor = gRenderTarget[DispatchRaysIndex().xy];
    float4 finalColor = (lastFrameColor * gSceneCB.accumCount + payload.color) / (gSceneCB.accumCount + 1);
    gRenderTarget[DispatchRaysIndex().xy] = finalColor;
    gRenderDisplay[DispatchRaysIndex().xy] = finalColor;
}

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload) {
    payload.color = gSceneCB.bgColor;
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
    //float4 wsNormal = float4(hitNormal, length(hitPosition - gSceneCB.cameraPos.xyz));
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

    // calculate AO
    uint randSeed = InitRand(launchIndex.x + launchIndex.y * launchDim.x, gSceneCB.frameCount, 16);
    float ambientOcclusion = AORayGen(hitPosition, hitNormal, randSeed);

    payload.color.rgb = ambientOcclusion;
    payload.color.a = 1;
}

