struct Constants
{
	uint baseTransformIndex;
};

struct CameraTransform
{
	float4x4 viewProjection;
	float4x4 view;
};

ConstantBuffer<Constants> constants : register(b0);
ConstantBuffer<CameraTransform> cameraTransform : register(b2);
StructuredBuffer<float4x3> transformsBuffer : register(t0);

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct VSOutput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
};

VSOutput main(VSInput input)
{
	uint baseTransformIndex = constants.baseTransformIndex;

	float4x3 transform = transformsBuffer[baseTransformIndex];
	float3 worldSpacePosition = mul(float4(input.position, 1.0f), transform);
	float3 worldSpaceNormal = mul(input.normal, (float3x3) transform);

	VSOutput output;
	output.position = mul(cameraTransform.viewProjection, float4(worldSpacePosition, 1.0f));
	output.normal = normalize(mul((float3x3) cameraTransform.view, worldSpaceNormal));
	return output;
}