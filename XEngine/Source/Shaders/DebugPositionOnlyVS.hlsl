cbuffer Constants : register(b0)
{
	float4x4 viewProjection;
	float4x4 view;
};

struct VSInput
{
	float3 position : POSITION;
	float4x3 transform : TRANSFORM;
};

struct VSOutput
{
	float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	float3 worldPosition = mul(float4(input.position, 1.0f), input.transform);
	output.position = mul(viewProjection, float4(worldPosition, 1.0f));
	return output;
}