
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
        MatExtra,
        OutputCount
    };

    static const uint InstanceMask = 0xff;   // Everything is visible.
    static const uint HitGroupIndex[RayTypeCount] = { 0, 1 };
    static const uint MissIndex[RayTypeCount] = { 0, 1 };

    static const float AlphaThreshold = 0.5f;
}

typedef BuiltInTriangleIntersectionAttributes Attributes;

RaytracingAccelerationStructure RtScene : register(t0, space0);

ConstantBuffer<SceneConstants> SceneCB : register(b0);

ByteAddressBuffer Indices : register(t1, space0);
StructuredBuffer<Vertex> Vertices : register(t2, space0);
StructuredBuffer<Geometry> Geometries : register(t3, space0);

RWTexture2D<float4> RenderTarget : register(u0);

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(out float3 origin, out float3 direction) {
    float2 pixelCenter = (DispatchRaysIndex().xy + float2(0.5f, 0.5f)) / DispatchRaysDimensions().xy; 
    float2 ndc = float2(2, -2) * pixelCenter + float2(-1, 1);                    
    direction = normalize(ndc.x * SceneCB.cameraU.xyz + ndc.y * SceneCB.cameraV.xyz + SceneCB.cameraW.xyz);
    origin = SceneCB.cameraPos.xyz;
}

inline bool AlphaTestFails(Attributes attribs) {
    return false;
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
inline float3 LerpVertexAttributes(float3 vertexAttributes[3], Attributes attr) {
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

    TraceRay(RtScene, 
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 
             RayTraceParams::InstanceMask, 
             RayTraceParams::HitGroupIndex[RayTraceParams::Primary], 
             0, 
             RayTraceParams::MissIndex[RayTraceParams::Primary], 
             ray, 
             payload);

    // Write the raytraced result to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = payload.color;
}

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload) {
    payload.color = SceneCB.bgColor;
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
    float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    uint indexOffset = Geometries[InstanceID()].indexInfo.x + PrimitiveIndex() * 3; // indicesPerTriangle(3)
    uint3 indices = Indices.Load3(indexOffset * 4); // indexSizeInBytes(4)

    float3 vertexNormals[3] = {
        Vertices[indices.x].normal,
        Vertices[indices.y].normal,
        Vertices[indices.z].normal
    };
    float3 hitNormal = LerpVertexAttributes(vertexNormals, attribs);
    hitNormal = normalize(mul((float3x3) ObjectToWorld3x4(), hitNormal));

    hitNormal = hitNormal * 0.5 + 0.5;

    payload.color = float4(hitNormal, 1);
}
