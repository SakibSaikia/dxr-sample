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
	float3 pos		: IA_POSITION;
	float3 normal	: IA_NORMAL;
	float2 uv		: IA_TEXCOORD0;
};

struct VSOut
{
	float4 ndcPos	: SV_POSITION;
	float4 normal	: INTERP_NORMAL;
	float2 uv		: INTERP_TEXCOORD0;
};

VSOut main( VSIn v )
{
	VSOut o;
	float4 worldPos = mul(float4(v.pos, 1.f), localToWorldMatrix);
	o.ndcPos = mul(worldPos, viewProjectionMatrix);
	o.normal = mul(float4(v.normal, 0.f), viewMatrix);
	o.uv = v.uv;

	return o;
}