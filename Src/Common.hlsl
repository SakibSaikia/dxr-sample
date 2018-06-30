cbuffer ViewConstants : register(b0)
{
	float4x4 viewMatrix;
	float4x4 viewProjectionMatrix;
};

cbuffer LightConstants : register(b2)
{
    float3 lightDir;
    float _pad;
    float3 lightColor;
    float lightBrightness;
    float4x4 lightViewProjectionMatrix;
};