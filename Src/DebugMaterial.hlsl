#include "Common.hlsl"

#define args    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)" \
			    ", CBV(b0)" 
				

struct VsToPs
{
	float4 ndcPos	: SV_POSITION;
    float4 color    : INTERP_COLOR;
};

//-----------------------------------------------------------------------------------

struct VsIn
{
	float3 pos		    : POSITION;
	float3 color	    : COLOR;
};

[RootSignature(args)]
VsToPs vs_main( VsIn v )
{
	VsToPs o;

	float4 worldPos = float4(v.pos, 1.f);
	o.ndcPos = mul(worldPos, viewProjectionMatrix);
    o.color = float4(v.color, 1.f);

	return o;
}


//-----------------------------------------------------------------------------------

[RootSignature(args)]
float4 ps_main(VsToPs p) : SV_TARGET
{
    return p.color;
}