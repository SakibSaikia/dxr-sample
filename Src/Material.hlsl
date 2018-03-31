#include "Common.hlsl"

#define commonArgs  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)" \
				    ", CBV(b0)" \
				    ", CBV(b1, visibility = SHADER_VISIBILITY_VERTEX)" \
				    ", StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL, filter = FILTER_ANISOTROPIC, maxAnisotropy = 8, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE)"

#ifdef NORMAL_MAPPED
    #ifdef MASKED
       #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 3), visibility = SHADER_VISIBILITY_PIXEL)"
    #else
       #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL)"
    #endif 
#elif defined(MASKED)
    #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL)"
#else
    #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)"
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

Texture2D diffuseMap : register(t0);
SamplerState anisoSampler : register(s0);

#ifdef NORMAL_MAPPED
    #ifdef MASKED
        Texture2D maskTex : register(t1);
        Texture2D normalTex : register(t2);
    #else
        Texture2D normalTex : register(t1);
    #endif 
#elif defined(MASKED)
    Texture2D maskTex : register(t1);
#endif

[RootSignature(args)]
float4 ps_main(VsToPs p) : SV_TARGET
{
#ifdef MASKED
	float mask = maskTex.Sample(anisoSampler, p.uv).r;
	clip(mask - 0.5f);
#endif

#ifdef NORMAL_MAPPED
    float3 normalMap = normalTex.Sample(anisoSampler, p.uv).rgb;
    normalMap = 2.f * normalMap - 1.f;
    float3 normal = mul(normalMap, float3x3(p.tangent, p.bitangent, p.normal));
    normal = normalize(mul(normal, (float3x3) viewMatrix));
#else
    float3 normal = normalize(mul(p.normal, (float3x3) viewMatrix));
#endif

    float3 light = normalize(mul(float3(1.f, 1.f, 0.f), (float3x3) viewMatrix));
	float4 matColor = diffuseMap.Sample(anisoSampler, p.uv);
	return matColor * (max(dot(normal, light), 0.f) + 0.1f);
}