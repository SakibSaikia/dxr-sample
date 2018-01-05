cbuffer ViewConstants : register(b0)
{
	float4x4 viewMatrix;
	float4x4 viewProjectionMatrix;
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
	o.ndcPos = mul(float4(v.pos, 1.f), viewProjectionMatrix);
	o.normal = mul(float4(v.normal, 0.f), viewMatrix);

	return o;
}