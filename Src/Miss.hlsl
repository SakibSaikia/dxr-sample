#include "Common.hlsli"
#include "MaterialCommon.hlsli"

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    payload.color = 0.2f.xxx;
    payload.hitT = -1.f;
}