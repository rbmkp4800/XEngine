cbuffer Constants : register(b0)
{
	float4x4 view;
	float4x4 viewProjection;
};

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
	float4x3 transform : TRANSFORM;
};

struct VSOutput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	float3 worldPosition = mul(float4(input.position, 1.0f), input.transform);
	output.position = mul(viewProjection, float4(worldPosition, 1.0f));
	float3 normal = mul(input.normal, (float3x3)input.transform);
	output.normal = normalize(mul((float3x3)view, normal));
	output.tex = input.tex;
	return output;
}