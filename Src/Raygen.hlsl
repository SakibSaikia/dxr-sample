#include "Common.hlsli"

#define IMAGE_PLANE_OFFSET 1.f

ConstantBuffer<ViewConstants> cb_view : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float4> RTOutput : register(u0);

[shader("raygeneration")]
void Raygen()
{
    // Pixel UV
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    float2 uv = launchIndex / (float2)launchDim;

    // Convert to NDC
    float2 ndc;
    ndc.x = 2.f * uv.x - 1.f;
    ndc.y = -2.f * uv.y + 1.f;

    float4 originImagePlane = cb_view.viewMatrix._14_24_34_44 + IMAGE_PLANE_OFFSET * cb_view.viewMatrix._13_23_33_43;
    float4 p = originImagePlane + ndc.x * cb_view.fovScale.x * cb_view.viewMatrix._11_21_31_41 + ndc.y * cb_view.fovScale.y * cb_view.viewMatrix._12_22_32_42;

    RayDesc ray;
    ray.Origin = cb_view.viewMatrix._14_24_34;
    ray.Direction = normalize(p.xyz - cb_view.viewMatrix._14_24_34);
    ray.TMin = 0.1f;
    ray.TMax = 1000.f;

    HitInfo payload;
    payload.color = float3(1.f, 0.f, 0.f);
    payload.hitT = 0.f;

    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    RTOutput[launchIndex.xy] = float4(payload.color, 1.f);
}