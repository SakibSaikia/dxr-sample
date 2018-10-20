#include "Common.hlsl"

#define commonArgs  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)" \
				    ", CBV(b0)" \
				    ", CBV(b1, visibility = SHADER_VISIBILITY_VERTEX)" \
                    ", CBV(b2, visibility = SHADER_VISIBILITY_ALL)" \
                    ", CBV(b3, visibility = SHADER_VISIBILITY_VERTEX)" \
                    ", CBV(b4, visibility = SHADER_VISIBILITY_PIXEL)" \
				    ", StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL, filter = FILTER_ANISOTROPIC, maxAnisotropy = 8, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE)" \
                    ", StaticSampler(s1, visibility = SHADER_VISIBILITY_PIXEL, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE)" \
                    ", DescriptorTable(SRV(t0, space = 0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)"

#if !defined(UNTEXTURED)
    #if defined(MASKED)
        #define args	commonArgs \
				        ", DescriptorTable(SRV(t0, space = 1, numDescriptors = 5), visibility = SHADER_VISIBILITY_PIXEL)"
    #else
        #define args	commonArgs \
				        ", DescriptorTable(SRV(t0, space = 1, numDescriptors = 4), visibility = SHADER_VISIBILITY_PIXEL)"
    #endif
#else
    #define args        commonArgs
#endif
				

struct VsToPs
{
	float4 ndcPos	        : SV_POSITION;
    float4 viewPos          : INTERP_VIEWPOS;
	float3 normal	        : INTERP_NORMAL;
    float3 lightUVAndDepth  : INTERP_SHADOWINFO;
#if !defined(UNTEXTURED)
    float3 tangent          : INTERP_TANGENT;
    float3 bitangent        : INTERP_BITANGENT;
    float2 uv               : INTERP_UV;
#endif
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
	o.ndcPos        = mul(worldPos, viewProjectionMatrix);
    o.viewPos       = mul(worldPos, viewMatrix);
    o.normal        = mul(v.normal, (float3x3) localToWorldMatrix);
#if !defined(UNTEXTURED)
    o.tangent       = mul(v.tangent, (float3x3) localToWorldMatrix);
    o.bitangent     = mul(v.bitangent, (float3x3) localToWorldMatrix);
	o.uv            = v.uv;
#endif

    float4 lightPos = mul(worldPos, lightViewProjectionMatrix);
    lightPos /= lightPos.w;
    o.lightUVAndDepth.xy = lightPos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
    o.lightUVAndDepth.z = lightPos.z;

	return o;
}


//-----------------------------------------------------------------------------------

SamplerState anisoSampler : register(s0);
SamplerComparisonState shadowmapSampler : register(s1);
Texture2D shadowMap : register(t0, space0);

#if !defined(UNTEXTURED)
    Texture2D texBaseColor  : register(t0, space1);
    Texture2D texRoughness  : register(t1, space1);
    Texture2D texMetallic   : register(t2, space1);
    Texture2D texNormalmap  : register(t3, space1);

    #if defined(MASKED)
        Texture2D texOpacityMask : register(t4, space1);
    #endif
#endif

static const float3 F_0_dielectric = float3(0.04, 0.04, 0.04);

// Tonemap operator
// See : https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 TonemapACES(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float CalcShadowFactor(float2 lightUV, float lightDepth)
{
    if (any(lightUV < float2(0.f, 0.f)) || 
        any(lightUV > float2(1.f, 1.f)))
    {
        return 1.f;
    }
    else
    {
        float shadowFactor = 0.f;

        uint width, height;
        shadowMap.GetDimensions(width, height);

        float delta = 1.f / (float) width;

        const float2 offsets[9] =
        {
            float2(-delta, -delta), float2(0.f, -delta), float2(delta, -delta),
            float2(-delta, 0.f), float2(0.f, 0.f), float2(delta, 0.f),
            float2(-delta, delta), float2(0.f, delta), float2(delta, delta)
        };

        [unroll]
        for (int i = 0; i < 9; i++)
        {
            shadowFactor += shadowMap.SampleCmpLevelZero(shadowmapSampler, lightUV + offsets[i], lightDepth).r;
        }

        return shadowFactor / 9.f;
    }
}

[RootSignature(args)]
float4 ps_main(VsToPs p) : SV_TARGET
{

#if !defined(UNTEXTURED)
    #if defined(MASKED)
	    float mask = texOpacityMask.Sample(anisoSampler, p.uv).r;
	    clip(mask - 0.5f);
    #endif

    float3 baseColor = texBaseColor.Sample(anisoSampler, p.uv).rgb;
    float metallic = texMetallic.Sample(anisoSampler, p.uv).r;
    float roughness = texRoughness.Sample(anisoSampler, p.uv).r;

    float3 normalMap = texNormalmap.Sample(anisoSampler, p.uv).rgb;
    normalMap = 2.f * normalMap - 1.f;
    float3 normal = mul(normalMap, float3x3(p.tangent, p.bitangent, p.normal));
    normal = normalize(mul(normal, (float3x3) viewMatrix));
#else
    float3 baseColor = mtlBaseColor;
    float metallic = mtlMetallic;
    float roughness = mtlRoughness;
    float3 normal = normalize(p.normal);
#endif

    // lighting calculations in view space
    float3 viewVec = p.viewPos.xyz/p.viewPos.w;
    float3 lightVec = normalize(mul(lightDir, (float3x3) viewMatrix));
    float3 halfVec = normalize(lightVec + viewVec);
    float3 reflectanceF0 = lerp(F_0_dielectric, baseColor.rgb, metallic);
    float3 albedo = (1.f - metallic) * baseColor.rgb;

    float nDotL = max(dot(normal, lightVec), 0.f);
    float nDotH = max(dot(normal, halfVec), 0.f);
    float nDotV = max(dot(normal, viewVec), 0.f);

    float3 reflectance = reflectanceF0 + (1.f - reflectanceF0) * pow((1 - nDotV), 5.f);

    // specular
    float3 specularBRDF = reflectance * 0.125 * (roughness + 8.f) * pow(nDotH, roughness);

    // diffuse + energy conservation
    float3 diffuseBRDF = (1.f - specularBRDF) * albedo;

    // Attenuation
    float lightAttenuation = CalcShadowFactor(p.lightUVAndDepth.xy, p.lightUVAndDepth.z);

    // lit color
    float3 ambient = float3(.01f, .01f, .01f) * albedo;
    float3 outColor = lightAttenuation * nDotL * (diffuseBRDF + specularBRDF) + ambient;

    // tonemap
    outColor = TonemapACES(outColor);

    // gamma correction
    outColor = pow(outColor, 1/2.2f);

    return float4(outColor, 1.f);
}