#pragma pack_matrix( row_major )

cbuffer ViewConstants : register(b0)
{
	float4x4 viewMatrix;
	float4x4 viewProjectionMatrix;
};

cbuffer SceneConstants : register(b1)
{
	float4x4 localToWorldMatrix;
};

struct VSIn
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
};

struct VSOut
{
	float4 ndcPos : SV_POSITION;
	float4 normal : NORMAL;
};

VSOut main( VSIn v )
{
	VSOut o;
	float4 worldPos = float4(v.pos, 1.f);// mul(float4(v.pos, 1.f), localToWorldMatrix);
	o.ndcPos = mul(worldPos, viewProjectionMatrix);
	o.normal = mul(float4(v.normal, 0.f), viewMatrix);

	return o;
}