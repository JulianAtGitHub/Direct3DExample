
struct Vertex {
    float3 position;
    float2 texCoord;
    float3 normal;
    float3 tangent;
    float3 bitangent;
};

struct Geometry {
    uint4 indexInfo; // x: index offset, y: index count;
    uint4 texInfo;  // x: diffuse, y: specular, z: normal
};

struct SceneConstants {
    float4 cameraPos;
    float4 cameraU;
    float4 cameraV;
    float4 cameraW;
    float4 bgColor;
};

struct PrimaryRayPayload {
    float4 color;
};

namespace RayTraceParams {
    enum RayType {
        Primary = 0,
        Shadow,
        RayTypeCount
    };

    enum Outputs {
        WsPosition = 0,
        WsNormal,
        MatDiffuse,
        MatSpecular,
        MatEmissive,
        MatExtra,
        OutputCount
    };

    static const uint InstanceMask = 0xff;   // Everything is visible.
    static const uint HitGroupIndex[RayTypeCount] = { 0, 1 };
    static const uint MissIndex[RayTypeCount] = { 0, 1 };

    static const float AlphaThreshold = 0.5f;
}

typedef BuiltInTriangleIntersectionAttributes Attributes;

RaytracingAccelerationStructure gRtScene : register(t0, space0);

ConstantBuffer<SceneConstants> gSceneCB : register(b0);

ByteAddressBuffer gIndices : register(t1, space0);
StructuredBuffer<Vertex> gVertices : register(t2, space0);
StructuredBuffer<Geometry> gGeometries : register(t3, space0);
Texture2D<float4> gMatTextures[] : register(t4);

SamplerState gSampler : register(s0);

RWTexture2D<float4> gRenderTarget : register(u0);
RWTexture2D<float4> gOutputs[] : register(u1);

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(out float3 origin, out float3 direction) {
    float2 pixelCenter = (DispatchRaysIndex().xy + float2(0.5f, 0.5f)) / DispatchRaysDimensions().xy; 
    float2 ndc = float2(2, -2) * pixelCenter + float2(-1, 1);                    
    direction = normalize(ndc.x * gSceneCB.cameraU.xyz + ndc.y * gSceneCB.cameraV.xyz + gSceneCB.cameraW.xyz);
    origin = gSceneCB.cameraPos.xyz;
}

inline bool AlphaTestFails(Attributes attribs) {
    return false;
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

[shader("raygeneration")]
void RayGener() {
    float3 rayPos;
    float3 rayDir;
    GenerateCameraRay(rayPos, rayDir);

    RayDesc ray;
    ray.Origin = rayPos;
    ray.Direction = rayDir;
    ray.TMin = 0.001;
    ray.TMax = 1000.0f;

    PrimaryRayPayload payload;
    payload.color = float4(0, 0, 0, 0);

    TraceRay(gRtScene, 
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::Primary], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::Primary], 
             ray, 
             payload);

    // Write the raytraced result to the output texture.
    gRenderTarget[DispatchRaysIndex().xy] = payload.color;
}

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload) {
    payload.color = gSceneCB.bgColor;
}

[shader("anyhit")]
void PrimaryAnyHit(inout PrimaryRayPayload payload, Attributes attribs) {
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (AlphaTestFails(attribs)) {
        IgnoreHit();
    }
}

[shader("closesthit")]
void PrimaryClosestHit(inout PrimaryRayPayload payload, in Attributes attribs) {
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint indexOffset = gGeometries[InstanceID()].indexInfo.x + PrimitiveIndex() * 3; // indicesPerTriangle(3)
    uint3 idx = gIndices.Load3(indexOffset * 4); // indexSizeInBytes(4)

    // position
    float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float4 wsPosition = float4(hitPosition, 1.0f);
    gOutputs[RayTraceParams::WsPosition][launchIndex] = wsPosition;

    // normal
    float3 normals[3] = { gVertices[idx.x].normal, gVertices[idx.y].normal, gVertices[idx.z].normal };
    float3 hitNormal = LerpFloat3Attributes(normals, attribs);
    hitNormal = normalize(mul((float3x3) ObjectToWorld3x4(), hitNormal));
    float4 wsNormal = float4(hitNormal, length(hitPosition - gSceneCB.cameraPos.xyz));
    gOutputs[RayTraceParams::WsNormal][launchIndex] = wsNormal;

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
    uint diffTexIdx = gGeometries[InstanceID()].texInfo.x;
    uint specTexIdx = gGeometries[InstanceID()].texInfo.y;
    float4 baseColor = gMatTextures[diffTexIdx].SampleLevel(gSampler, hitTexCoord, 0);
    float4 spec = gMatTextures[specTexIdx].SampleLevel(gSampler, hitTexCoord, 0);

    // diffuse 
    float4 diffColor = float4(lerp(baseColor.rgb, float3(0, 0, 0), spec.b), baseColor.a);
    gOutputs[RayTraceParams::MatDiffuse][launchIndex] = diffColor;
    // specular UE4 uses 0.08 multiplied by a default specular value of 0.5 as a base, hence the 0.04
    // Clamp the roughness so that the BRDF won't explode
    float4 specColor = float4(lerp(float3(0.04f, 0.04f, 0.04f), baseColor.rgb, spec.b), max(0.08, spec.g));
    gOutputs[RayTraceParams::MatSpecular][launchIndex] = specColor;

    // other mats
    gOutputs[RayTraceParams::MatEmissive][launchIndex] = float4(0, 0, 0, 0);
    gOutputs[RayTraceParams::MatExtra][launchIndex] = float4(1, 0, 0, 0);

    payload.color = wsNormal;
}
