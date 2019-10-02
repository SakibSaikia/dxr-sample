#include "Common.hlsl"

#define IMAGE_PLANE_OFFSET 1.f

RayTracingAccelerationStructure SceneBVH : register(t0);

[GeometryShader("raygeneration")]
void main()
{
    // Pixel UV
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    float2 uv = launchIndex / (float2)launchDim;

    // Convert to NDC
    float2 ndc;
    ndc.x = 2.f * uv.x - 1.f;
    ndc.y = -2.f * uv.y + 1.f;

    float3 originImagePlane = cb_view.origin + IMAGE_PLANE_OFFSET * cb_view.forwardDir;
    float3 p = originImagePlane + ndc.x * cb_view.fovScale.x * cb_view.rightDir + ndc.y * cb_view.fovScale.y * cb_view.upDir;

    RayDesc ray;
    ray.Origin = cb_view.origin;
    ray.Direction = normalize(p - cb_view.origin);
    ray.TMin = 0.1f;
    ray.TMax = 1000.f;

    HitInfo payload;
    payload.color = float3(1.f, 0.f, 0.f);
    payload.hitT = 0.f;

    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

    RTOutput[launchIndex.xy] = float4(payload.color, 1.f);
}