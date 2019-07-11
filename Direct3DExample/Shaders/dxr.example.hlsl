
RaytracingAccelerationStructure gRTScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);

// Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
float3 LinearToSRGB(float3 c)
{
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

[shader("raygeneration")]
void RayGeneration() {
    uint3 launchIndex = DispatchRaysIndex();
    float3 color = LinearToSRGB(float3(0.4, 0.6, 0.2));
    gOutput[launchIndex.xy] = float4(color, 1);
}

struct Payload {
    bool hit;
};

[shader("miss")]
void Miss(inout Payload payload) {
    payload.hit = false;
}

[shader("closesthit")]
void ClosestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attribs) {
    payload.hit = true;
}
