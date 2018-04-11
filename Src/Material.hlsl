#include "Common.hlsl"

#define commonArgs  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)" \
				    ", CBV(b0)" \
				    ", CBV(b1, visibility = SHADER_VISIBILITY_VERTEX)" \
				    ", StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL, filter = FILTER_ANISOTROPIC, maxAnisotropy = 8, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE)"

#if defined(MASKED)
    #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 5), visibility = SHADER_VISIBILITY_PIXEL)"
#else
    #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 4), visibility = SHADER_VISIBILITY_PIXEL)"
#endif
				

struct VsToPs
{
	float4 ndcPos	: SV_POSITION;
    float3 tangent  : INTERP_TANGENT;
    float3 bitangent: INTERP_BITANGENT;
	float3 normal	: INTERP_NORMAL;
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
    o.tangent = mul(v.tangent, (float3x3) localToWorldMatrix);
    o.bitangent = mul(v.bitangent, (float3x3) localToWorldMatrix);
    o.normal = mul(v.normal, (float3x3) localToWorldMatrix);
	o.uv = v.uv;

	return o;
}


//-----------------------------------------------------------------------------------

SamplerState anisoSampler : register(s0);

Texture2D texBaseColor : register(t0);
Texture2D texRoughness : register(t1);
Texture2D texMetallic : register(t2);
Texture2D texNormalmap : register(t3);
#if defined(MASKED)
    Texture2D texOpacityMask : register(t4);
#endif

[RootSignature(args)]
float4 ps_main(VsToPs p) : SV_TARGET
{
#ifdef MASKED
	float mask = texOpacityMask.Sample(anisoSampler, p.uv).r;
	clip(mask - 0.5f);
#endif

    float3 normalMap = texNormalmap.Sample(anisoSampler, p.uv).rgb;
    normalMap = 2.f * normalMap - 1.f;
    float3 normal = mul(normalMap, float3x3(p.tangent, p.bitangent, p.normal));
    normal = normalize(mul(normal, (float3x3) viewMatrix));

    float3 light = normalize(mul(float3(1.f, 1.f, 0.f), (float3x3) viewMatrix));
    float4 matColor = texBaseColor.Sample(anisoSampler, p.uv);
	return matColor * (max(dot(normal, light), 0.f) + 0.1f);
}