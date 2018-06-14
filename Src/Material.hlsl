#include "Common.hlsl"

#define commonArgs  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)" \
				    ", CBV(b0)" \
				    ", CBV(b1, visibility = SHADER_VISIBILITY_VERTEX)" \
                    ", CBV(b2, visibility = SHADER_VISIBILITY_PIXEL)" \
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

cbuffer LightConstants : register(b2)
{
    float3 lightDir;
    float3 lightColor;
    float lightBrightness;
};

SamplerState anisoSampler : register(s0);

Texture2D texBaseColor : register(t0);
Texture2D texRoughness : register(t1);
Texture2D texMetallic : register(t2);
Texture2D texNormalmap : register(t3);
#if defined(MASKED)
    Texture2D texOpacityMask : register(t4);
#endif

static const float3 F_0_dielectric = float3(0.04, 0.04, 0.04);

// Tonemap operator
// See : https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 tonemapACES(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[RootSignature(args)]
float4 ps_main(VsToPs p) : SV_TARGET
{
#ifdef MASKED
	float mask = texOpacityMask.Sample(anisoSampler, p.uv).r;
	clip(mask - 0.5f);
#endif

    float3 baseColor = texBaseColor.Sample(anisoSampler, p.uv).rgb;

    float3 normalMap = texNormalmap.Sample(anisoSampler, p.uv).rgb;
    normalMap = 2.f * normalMap - 1.f;
    float3 normal = mul(normalMap, float3x3(p.tangent, p.bitangent, p.normal));
    normal = normalize(mul(normal, (float3x3) viewMatrix));

    float3 light = normalize(mul(lightDir, (float3x3) viewMatrix));

    float3 halfVector = normalize(light + float3(0, 0, 1));

    float metallic = texMetallic.Sample(anisoSampler, p.uv).r;

    float roughness = texRoughness.Sample(anisoSampler, p.uv).r;

    float3 F_0 = lerp(baseColor.rgb, F_0_dielectric, metallic);

    float nDotL = max(dot(normal, light), 0.f);

    float lDotH = max(dot(light, halfVector), 0.f);

    float3 reflectance = F_0 + (1.f - F_0) * pow((1 - nDotL), 5.f);

    // diffuse
    float3 outColor = (1.f - metallic) * nDotL * baseColor.rgb;

    // specular
    outColor += nDotL * reflectance * 0.125 * (roughness + 8.f) * pow(lDotH, roughness);

    // ambient
    outColor += float3(.1f, .1f, .1f) * baseColor.rgb;

    // tonemap
    outColor = tonemapACES(outColor);

    // gamma correction
    //outColor = pow(outColor, 1/2.2f);

    return float4(outColor, 1.f);
}