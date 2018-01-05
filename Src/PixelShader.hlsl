struct PSIn
{
	float4 ndcPos : SV_POSITION;
	float4 normal : NORMAL;
};

float4 main(PSIn p) : SV_TARGET
{
	float4 normal = normalize(p.normal);
	return dot(normal, float4(0.f, 1.f, 0.f, 0.f)) + 0.5f;
}