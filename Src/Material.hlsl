#include "Common.hlsl"

struct VsToPs
{
	float4 ndcPos	: SV_POSITION;
	float4 normal	: INTERP_NORMAL;
	float2 uv		: INTERP_TEXCOORD0;
};

//-----------------------------------------------------------------------------------

cbuffer SceneConstants : register(b1)
{
	float4x4 localToWorldMatrix;
};

struct VsIn
{
	float3 pos		: IA_POSITION;
	float3 normal	: IA_NORMAL;
	float2 uv		: IA_TEXCOORD0;
};

VsToPs vs_main( VsIn v )
{
	VsToPs o;
	float4 worldPos = mul(float4(v.pos, 1.f), localToWorldMatrix);
	o.ndcPos = mul(worldPos, viewProjectionMatrix);
	o.normal = mul(float4(v.normal, 0.f), viewMatrix);
	o.uv = v.uv;

	return o;
}


//-----------------------------------------------------------------------------------

Texture2D diffuseMap : register(t0);
SamplerState anisoSampler : register(s0);

float4 ps_main(VsToPs p) : SV_TARGET
{
	float4 normal = normalize(p.normal);
	float4 light = normalize(mul(float4(1.f, 1.f, 0.f, 0.f), viewMatrix));
	float4 matColor = diffuseMap.Sample(anisoSampler, p.uv);
	return matColor * (max(dot(normal, light), 0.f) + 0.1f);
}