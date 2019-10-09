struct ObjectConstants
{
    uint vertexStride;
};

struct ViewConstants
{
    float4x4 viewMatrix;
    float2 fovScale;
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

struct Attributes
{
    float2 barycentricUV;
};