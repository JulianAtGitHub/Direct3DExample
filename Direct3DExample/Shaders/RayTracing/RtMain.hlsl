
#include "PrimaryRay.hlsli"
#include "IndirectRay.hlsli"
#include "ShadowRay.hlsli"

[shader("raygeneration")]
void RayGener() {

    float3 color = PrimaryRayGen();

    uint2 launchIdx = DispatchRaysIndex().xy;
    if (gSettingsCB.enableAccumulate) {
        float4 lastFrameColor = gRenderTarget[launchIdx];
        float3 finalColor = (lastFrameColor.rgb * gSceneCB.accumCount + color) / (gSceneCB.accumCount + 1);
        gRenderTarget[launchIdx] = float4(finalColor, 1);
        gRenderDisplay[launchIdx] = float4(finalColor, 1);
    } else {
        gRenderTarget[launchIdx] = float4(color, 1);
        gRenderDisplay[launchIdx] = float4(color, 1);
    }
}

