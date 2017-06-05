cbuffer Constants : register(b0)
{
	float4x4 viewProjection;
	float4x4 view;
};

//StructuredBuffer<float3x4> jointTransforms : register(t0);

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float4 jointWeights : JOINTWEIGHTS;
	uint4 jointIndices : JOINTINDICES;
	float4x3 transform : TRANSFORM;
};

struct VSOutput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 color : COLOR;
};

float3 getColorFromJointIndex(uint index)
{
	switch (index)
	{
	case 0: return float3(0.0f, 0.0f, 1.0f);
	case 1: return float3(0.0f, 1.0f, 0.0f);
	case 2: return float3(0.0f, 1.0f, 1.0f);
	case 3: return float3(1.0f, 0.0f, 0.0f);
	case 4: return float3(1.0f, 0.0f, 1.0f);
	case 5: return float3(1.0f, 1.0f, 0.0f);
	case 6: return float3(1.0f, 1.0f, 1.0f);
	default: return float3(0.5f, 0.5f, 0.5f);
	}
}

VSOutput main(VSInput input)
{
	VSOutput output;
	float3 worldPosition = mul(float4(input.position, 1.0f), input.transform);
	output.position = mul(viewProjection, float4(worldPosition, 1.0f));
	float3 normal = mul(input.normal, (float3x3)input.transform);
	output.normal = normalize(mul((float3x3)view, normal));

	float3 color = getColorFromJointIndex(input.jointIndices.x) * input.jointWeights.x;
	color += getColorFromJointIndex(input.jointIndices.y) * input.jointWeights.y;
	output.color = color;

	return output;
}