#include "Common.hlsl"

#define commonArgs  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)" \
				    ", CBV(b0)" \
				    ", CBV(b1, visibility = SHADER_VISIBILITY_VERTEX)" \
				    ", StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL, filter = FILTER_ANISOTROPIC, maxAnisotropy = 8, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE)"

#ifdef MASKED
    #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL)"
#else
    #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)"
#endif
				

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
	float3 pos		    : POSITION;
	float3 normal	    : NORMAL;
    float3 tangent      : TANGENT;
    float3 bitangent    : BITANGENT;
	float2 uv		    : TEXCOORD0;
};

[RootSignature(args)]
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

#if MASKED
Texture2D maskTex : register(t1);
#endif

[RootSignature(args)]
float4 ps_main(VsToPs p) : SV_TARGET
{
#if MASKED
	float mask = maskTex.Sample(anisoSampler, p.uv);
	clip(mask - 0.5f);
#endif

	float4 normal = normalize(p.normal);
	float4 light = normalize(mul(float4(1.f, 1.f, 0.f, 0.f), viewMatrix));
	float4 matColor = diffuseMap.Sample(anisoSampler, p.uv);
	return matColor * (max(dot(normal, light), 0.f) + 0.1f);
}