cbuffer ViewConstants : register(b0)
{
	float4x4 viewMatrix;
	float4x4 viewProjectionMatrix;
};

cbuffer LightConstants : register(b2)
{
    float3 lightDir         : packoffset(c0);
    float3 lightColor       : packoffset(c1);
    float lightBrightness   : packoffset(c1.w);
};

// same size as ViewConstants so that we can switch it out in the DepthRendering shader to render shadowmaps
cbuffer ShadowConstants : register(b3)
{
    float4x4 lightViewMatrix;
    float4x4 lightViewProjectionMatrix;
};

cbuffer MaterialConstants : register(b4)
{
    float3 mtlBaseColor    : packoffset(c0);
    float mtlMetallic      : packoffset(c0.w);
    float mtlRoughness     : packoffset(c1);
};