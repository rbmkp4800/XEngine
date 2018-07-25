struct Constants
{
	uint baseTransformIndex;
};

struct CameraTransform
{
	float4x4 transform;
};

ConstantBuffer<Constants> constants : register(b0);
ConstantBuffer<CameraTransform> cameraTransform : register(b2);
StructuredBuffer<float4x3> transformsBuffer : register(t0);

struct VSInput
{
	float3 position : POSITION;
};

struct VSOutput
{
	float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
	uint baseTransformIndex = constants.baseTransformIndex;

	float4x3 transform = transformsBuffer[baseTransformIndex];
	float3 worldSpacePosition = mul(float4(input.position, 1.0f), transform);

	VSOutput output;
	output.position = mul(cameraTransform.transform, float4(worldSpacePosition, 1.0f));
	return output;
}