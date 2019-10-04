#include "Common.hlsli"
#include "MaterialCommon.hlsli"

[shader("closesthit")]
void ClosestHit(inout HitInfo payload : SV_RayPayload, float2 barycentricUV : SV_IntersectionAttributes)
{
#if !defined(UNTEXTURED)
    uint triangleIndex = PrimitiveIndex();
    float3 barycentricCoords = float3(1.f - barycentricUV.x - barycentricUV.y, barycentricUV.x, barycentricUV.y);
    VertexAttributes vertex = GetVertexAttributes(triangleIndex, barycentricCoords);
    float3 baseColor = texBaseColor.Sample(anisoSampler, vertex.uv).rgb;
#else
    float3 baseColor = cb_material.baseColor;
#endif

    payload.color = baseColor;
    payload.hitT = RayTCurrent();
}