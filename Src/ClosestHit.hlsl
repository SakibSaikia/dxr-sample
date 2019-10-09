#include "Common.hlsli"
#include "MaterialCommon.hlsli"

[shader("closesthit")]
void ClosestHit(inout HitInfo payload : SV_RayPayload, Attributes attrib : SV_IntersectionAttributes)
{
#if !defined(UNTEXTURED)
    uint triangleIndex = PrimitiveIndex();
    float3 barycentricCoords = float3(1.f - attrib.barycentricUV.x - attrib.barycentricUV.y, attrib.barycentricUV.x, attrib.barycentricUV.y);
    VertexAttributes vertex = GetVertexAttributes(triangleIndex, barycentricCoords);

    float2 textureDim;
    texBaseColor.GetDimensions(textureDim.x, textureDim.y);

    float3 baseColor = texBaseColor.Load(uint3(vertex.uv * textureDim, 0)).rgb;
#else
    float3 baseColor = cb_material.baseColor;
#endif

    payload.color = baseColor;
    payload.hitT = RayTCurrent();
}