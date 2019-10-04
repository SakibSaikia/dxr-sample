struct ObjectConstants
{
    uint vertexStride;
};

struct ViewConstants
{
    float4 origin;
    float4 up;
    float4 right;
    float4 look;
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