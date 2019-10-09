ConstantBuffer<ObjectConstants> cb_object : register(b0);
ConstantBuffer<ViewConstants> cb_view : register(b1);
ConstantBuffer<LightConstants> cb_light : register(b2);
#if defined(UNTEXTURED)
    ConstantBuffer<MaterialConstants> cb_material   : register(b3);
#endif

ByteAddressBuffer vertices : register(t0, space0);
ByteAddressBuffer indices : register(t1, space0);

#if !defined(UNTEXTURED)
    Texture2D texBaseColor : register(t0, space1);
    Texture2D texRoughness : register(t1, space1);
    Texture2D texMetallic : register(t2, space1);
    Texture2D texNormalmap : register(t3, space1);
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
        int address = (indices[i] * cb_object.vertexStride) * 4;
        v.position += asfloat(vertices.Load3(address)) * barycentrics[i];
        address += 3 * 4;
        v.uv += asfloat(vertices.Load2(address)) * barycentrics[i];
    }

    return v;
}