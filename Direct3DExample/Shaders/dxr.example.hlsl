
struct SceneConstantBuffer {
    float4x4 projectionToWorld;
    float4 cameraPosition;
    float4 lightDirection;
    float4 lightAmbientColor;
    float4 lightDiffuseColor;
};

struct MeshConstantBuffer {
    float4 albedo[2];
    uint2 offset;
};

struct Vertex {
    float3 position;
    float3 normal;
    uint value;
};

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);
ByteAddressBuffer Indices : register(t1, space0);
StructuredBuffer<Vertex> Vertices : register(t2, space0);

ConstantBuffer<SceneConstantBuffer> g_sceneCB : register(b0);
ConstantBuffer<MeshConstantBuffer> g_meshCB : register(b1);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload {
    float4 color;
};

struct ShadowRayPayload {
    bool hit;
};

// Retrieve hit world position.
float3 HitWorldPosition() {
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr) {
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction) {
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), g_sceneCB.projectionToWorld);

    world.xyz /= world.w;
    origin = g_sceneCB.cameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

// Diffuse lighting calculation.
float4 CalculateDiffuseLighting(float3 hitPosition, float3 normal, uint value) {
    // Diffuse contribution.
    float fNDotL = max(0.0f, dot(-g_sceneCB.lightDirection.xyz, normal));
    return g_meshCB.albedo[value] * g_sceneCB.lightDiffuseColor * fNDotL;
}

[shader("raygeneration")]
void MyRaygenShader() {
    float3 rayDir;
    float3 origin;

    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr) {
    float3 hitPosition = HitWorldPosition();

    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = hitPosition;
    ray.Direction = -g_sceneCB.lightDirection.xyz;
    // Set TMin to a zero value to avoid aliasing artifcats along contact areas.
    // Note: make sure to enable back-face culling so as to avoid surface face fighting.
    ray.TMin = 0.001;
    ray.TMax = 10000;
    uint rayFlags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

    ShadowRayPayload shadowPayload = { true };
    TraceRay(Scene, rayFlags, ~0, 1, 1, 1, ray, shadowPayload);

    const uint triangleIndexStride = 12; // indicesPerTriangle(3) * indexSizeInBytes(4)
    const uint primitiveOffset = g_meshCB.offset[InstanceID()] + PrimitiveIndex();
    const uint3 indices = Indices.Load3(primitiveOffset * triangleIndexStride);
    const uint value = Vertices[indices.x].value;

    if (!shadowPayload.hit) {

        // Retrieve corresponding vertex normals for the triangle vertices.
        float3 vertexNormals[3] = {
            Vertices[indices.x].normal,
            Vertices[indices.y].normal,
            Vertices[indices.z].normal
        };

        // Compute the triangle's normal.
        // This is redundant and done for illustration purposes 
        // as all the per-vertex normals are the same and match triangle's normal in this sample. 
        float3 triangleNormal = HitAttribute(vertexNormals, attr);
        triangleNormal = normalize(mul((float3x3) ObjectToWorld3x4(), triangleNormal));

        float4 diffuseColor = CalculateDiffuseLighting(hitPosition, triangleNormal, value);
        float4 color = g_sceneCB.lightAmbientColor + diffuseColor;

        payload.color = color;
    } else {
        payload.color = float4(g_meshCB.albedo[value].xyz * g_sceneCB.lightAmbientColor.xyz, 1.0);
    }
}

[shader("miss")]
void MyMissShader(inout RayPayload payload) {
    payload.color = float4(0.0f, 0.2f, 0.4f, 1.0f);
}

[shader("miss")]
void MyMissShadow(inout ShadowRayPayload payload) {
    payload.hit = false;
}

