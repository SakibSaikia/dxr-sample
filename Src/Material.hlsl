#include "Common.hlsl"

ByteAddressBuffer indices : register(t0, space0);
ByteAddressBuffer vertices : register(t1, space0);

SamplerState anisoSampler : register(s0);

#if !defined(UNTEXTURED)
    Texture2D texBaseColor          : register(t0, space1);
    Texture2D texRoughness          : register(t1, space1);
    Texture2D texMetallic           : register(t2, space1);
    Texture2D texNormalmap          : register(t3, space1);

    #if defined(MASKED)
        Texture2D texOpacityMask    : register(t4, space1);
    #endif
#endif

struct VertexAttributes
{
    float3 position;
    float2 uv;
};

uint3 GetIndices(uint triangleIndex)
{
    uint baseIndex = triangleIndex * 3;
    int address = baseIndex * 4;
    return indices.Load3(address);
}

VertexAttributes GetVertexAttributes(uint triangleIndex, float3 barycentrics)
{
    uint3 indices = GetIndices(triangleIndex);
    VertexAttributes v;
    v.position = 0.f.xxx;
    v.uv = 0.f.xx;

    for (uint i = 0; i < 3; i++)
    {
        int address = (indices[i] * VERTEX_STRIDE) * 4;
        v.position += asfloat(vertices.Load3(address)) * barycentrics[i];
        address += 3 * 4;
        v.uv += asfloat(vertices.Load2(address)) * barycentrics[i];
    }

    return v;
}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload : SV_RayPayload, float2 barycentricUV : SV_IntersectionAttributes)
{
#if !defined(UNTEXTURED)
    uint triangleIndex = PrimitiveIndex();
    float3 barycentricCoords = float3(1.f - barycentricUV.x - barycentricUV.y, barycentricUV.x, barycentricUV.y);
    VertexAttributes vertex = GetVertexAttributes(triangleIndex, barycentricCoords);
    float3 baseColor = texBaseColor.Sample(anisoSampler, vertex.uv);
#else
    float3 baseColor = cb_material.baseColor;
#endif

    payload.color = baseColor;
    payload.hitT = RayTCurrent();
}

[GeometryShader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    payload.color = 0.2f.xxx;
    payload.hitT = -1.f;
}