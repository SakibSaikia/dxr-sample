#pragma pack_matrix( row_major )

Texture2D diffuseMap : register(t0);

cbuffer ViewConstants : register(b0)
{
	float4x4 viewMatrix;
	float4x4 viewProjectionMatrix;
};

struct PSIn
{
	float4 ndcPos : SV_POSITION;
	float4 normal : NORMAL;
};

float4 main(PSIn p) : SV_TARGET
{
	float4 normal = normalize(p.normal);
	float4 light = normalize(mul(float4(1.f, 1.f, 0.f, 0.f), viewMatrix));
	return max(dot(normal, light), 0.f) + 0.1f;
}