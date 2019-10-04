#include "Common.hlsli"

#define IMAGE_PLANE_OFFSET 1.f

ConstantBuffer<ViewConstants> cb_view : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float4> RTOutput : register(u0);

[shader("raygeneration")]
void Main()
{
    // Pixel UV
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    float2 uv = launchIndex / (float2)launchDim;

    // Convert to NDC
    float2 ndc;
    ndc.x = 2.f * uv.x - 1.f;
    ndc.y = -2.f * uv.y + 1.f;

    float4 originImagePlane = cb_view.origin + IMAGE_PLANE_OFFSET * cb_view.look;
    float4 p = originImagePlane + ndc.x * cb_view.fovScale.x * cb_view.right + ndc.y * cb_view.fovScale.y * cb_view.up;

    RayDesc ray;
    ray.Origin = cb_view.origin.xyz;
    ray.Direction = normalize(p - cb_view.origin).xyz;
    ray.TMin = 0.1f;
    ray.TMax = 1000.f;

    HitInfo payload;
    payload.color = float3(1.f, 0.f, 0.f);
    payload.hitT = 0.f;

    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

    RTOutput[launchIndex.xy] = float4(payload.color, 1.f);
}