#include "Common.hlsl"

#define commonArgs  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)" \
				    ", CBV(b1, visibility = SHADER_VISIBILITY_VERTEX)" \
                    ", CBV(b2, visibility = SHADER_VISIBILITY_VERTEX)" \

#if defined(MASKED)
    #define args	commonArgs \
				    ", DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)" \
                    ", StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL, filter = FILTER_ANISOTROPIC, maxAnisotropy = 8, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE)"
#else
    #define args    commonArgs
#endif
				

struct VsToPs
{
    float4 ndcPos   : SV_POSITION;
    float2 uv       : INTERP_TEXCOORD0;
};

//-----------------------------------------------------------------------------------

cbuffer SceneConstants : register(b1)
{
    float4x4 localToWorldMatrix;
};

struct VsIn
{
    float3 pos          : POSITION;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
    float3 bitangent    : BITANGENT;
    float2 uv           : TEXCOORD0;
};

[RootSignature(args)]
VsToPs vs_main(VsIn v)
{
    VsToPs o;
    float4 worldPos = mul(float4(v.pos, 1.f), localToWorldMatrix);
    o.ndcPos = mul(worldPos, lightViewProjectionMatrix);
    o.uv = v.uv;
    return o;
}


//-----------------------------------------------------------------------------------

#if defined(MASKED)
    Texture2D texOpacityMask : register(t0);
    SamplerState anisoSampler : register(s0);
#endif

[RootSignature(args)]
void ps_main(VsToPs p)
{
#ifdef MASKED
	float mask = texOpacityMask.Sample(anisoSampler, p.uv).r;
	clip(mask - 0.5f);
#endif
}