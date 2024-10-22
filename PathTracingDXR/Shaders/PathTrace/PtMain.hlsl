
#include "SharedTypes.h"

enum RayType {
    PrimaryRay = 0,
    IndirectRay,
    ShadowRay,
    RayTypeCount
};

enum LightType {
    DirectLight = 0,
    PointLight,
    SpotLight,
    LightTypeCount
};

struct HitSample {
    uint   matType;
    float3 position;
    float3 normal;
    float4 albedo;
    float3 emissive;
    float  roughness;
    float  refractivity;
};

struct LightSample {
    float3 L;
    float3 diffuse;
    float3 specular;
    float3 position;
};

struct GenericRayPayload {
    float3 color;
    uint   seed;
    uint   depth;
};

struct ShadowRayPayload {
    float value;
};

typedef GenericRayPayload PrimaryRayPayload;
typedef GenericRayPayload IndirectRayPayload;

typedef BuiltInTriangleIntersectionAttributes Attributes;

static const float M_PI = 3.14159265f;
static const float M_1_PI = 0.318309886f;
static const float FLT_MAX = 3.402823466e+38f;

static const uint TEX_INDEX_INVALID = 0xFFFFFFFF;
static const uint INSTANCE_MASK = 0xFF;   // Everything is visible.
static const float ALPHA_THRESHOLD = 0.5f;

static const uint HIT_GROUP_INDEX[RayTypeCount] = { 0, 1, 2 };
static const uint MISS_INDEX[RayTypeCount] = { 0, 1, 2 };

/****Input & Output*****************************************/

RaytracingAccelerationStructure gRtScene : register(t0, space0);
// buffers
ByteAddressBuffer gIndices : register(t1, space0);
StructuredBuffer<Vertex> gVertices : register(t2, space0);
StructuredBuffer<Geometry> gGeometries : register(t3, space0);
StructuredBuffer<Material> gMaterials : register(t4, space0);
StructuredBuffer<Light> gLights : register(t5, space0);
Texture2D<float4> gEnvTexture : register(t6);
Texture2D<float4> gMatTextures[] : register(t7);
// constant values
ConstantBuffer<AppSettings> gSettingsCB : register(b0);
ConstantBuffer<SceneConstants> gSceneCB : register(b1);
ConstantBuffer<CameraConstants> gCameraCB : register(b2);
// smapler
SamplerState gSampler : register(s0);
// output
RWTexture2D<float4> gRenderTarget : register(u0);

#include "PathTrace.hlsli"

[shader("raygeneration")]
void RayGener() {

    float3 color = PrimaryRayGen();

    uint2 launchIdx = DispatchRaysIndex().xy;
    if (gSettingsCB.enableAccumulate && gSceneCB.accumCount) {
        gRenderTarget[launchIdx] += float4(color, 1);
    } else {
        gRenderTarget[launchIdx] = float4(color, 1);
    }
}

