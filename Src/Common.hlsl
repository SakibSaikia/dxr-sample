struct ViewConstants
{
	float4x4 viewMatrix;
	float4x4 viewProjectionMatrix;
};

struct ObjectConstants
{
    float4x4 localToWorldMatrix;
};

struct LightConstants
{
    float3 lightDir;
    float _pad;
    float3 lightColor;
    float lightBrightness;
};

// same size as ViewConstants so that we can switch it out in the DepthRendering shader to render shadowmaps
struct ShadowConstants
{
    float4x4 lightViewMatrix;
    float4x4 lightViewProjectionMatrix;
};

struct MaterialConstants
{
    float3 baseColor;
    float metallic;
    float roughness;
};


ConstantBuffer<ViewConstants> cb_view : register(b0);
ConstantBuffer<ObjectConstants> cb_object : register(b1);
ConstantBuffer<LightConstants> cb_light : register(b2);
ConstantBuffer<ShadowConstants> cb_shadow : register(b3);
ConstantBuffer<MaterialConstants> cb_material : register(b4);