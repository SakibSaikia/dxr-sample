struct ViewConstants
{
    float3 origin;
};

struct LightConstants
{
    float3 lightDir;
    float _pad;
    float3 lightColor;
    float lightBrightness;
};

struct MaterialConstants
{
    float3 baseColor;
    float metallic;
    float roughness;
};

struct HitInfo
{
    float3 color : SHADED_COLOR;
    float hitT : HIT_T;
};

ConstantBuffer<ViewConstants> cb_view               : register(b0);
ConstantBuffer<LightConstants> cb_light             : register(b1);
#if defined(UNTEXTURED)
    ConstantBuffer<MaterialConstants> cb_material   : register(b2);
#endif

RWTexture2D<float4> RTOutput : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0); // Check for collision with mesh SRVs