#pragma pack_matrix( row_major )

Texture2D diffuseMap : register(t0);
SamplerState anisoSampler : register(s0);

cbuffer ViewConstants : register(b0)
{
	float4x4 viewMatrix;
	float4x4 viewProjectionMatrix;
};

struct PSIn
{
	float4 ndcPos : SV_POSITION;
	float4 normal : INTERP_NORMAL;
	float2 uv	  : INTERP_TEXCOORD0;
};

float4 main(PSIn p) : SV_TARGET
{
	float4 normal = normalize(p.normal);
	float4 light = normalize(mul(float4(1.f, 1.f, 0.f, 0.f), viewMatrix));
	float4 matColor = diffuseMap.Sample(anisoSampler, p.uv);
	return matColor * (max(dot(normal, light), 0.f) + 0.1f);
}