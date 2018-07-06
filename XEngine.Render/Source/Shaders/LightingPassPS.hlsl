static const float specularPower = 100.0f;
static const float abmientIntensity = 0.3f;

struct Light
{
	float3 viewSpacePosition;
	float3 color;
	float intensity;
};

cbuffer Constants : register(b0)
{
	float ndcToViewDepthConversionA;
	float ndcToViewDepthConversionB;
	float aspect;
	float halfFOVTan;
};

Texture2D<float4> diffuseTexture : register(t0);
Texture2D<float2> normalTexture : register(t1);
Texture2D<float> depthTexture : register(t2);

struct PSInput
{
	float4 position : SV_Position;
	float2 ndcPosition : POSITION0;
};

float sqr(float value) { return value * value; }

float NDCDepthToViewSpaceDepth(float ndcDepth)
{
	// result = (zFar * zNear) / (zFar - ndcDepth * (zFar - zNear));

	// optimized:
	// ndcToViewDepthConversionA = (zFar * zNear) / (zFar - zNear);
	// ndcToViewDepthConversionB = zFar / (zFar - zNear);

	return ndcToViewDepthConversionA / (ndcToViewDepthConversionB - ndcDepth);
}

float4 main(PSInput input) : SV_Target
{
	int2 texPosition = int2(input.position.xy);

	float ndcDepth = depthTexture[texPosition];

	float viewSpaceDepth = NDCDepthToViewSpaceDepth(ndcDepth);
	float3 viewSpacePosition = float3(input.ndcPosition * viewSpaceDepth * halfFOVTan, viewSpaceDepth);
	viewSpacePosition.x *= aspect;
	float3 normalizedViewSpacePosition = normalize(viewSpacePosition);

	if (ndcDepth == 1.0f)
	{
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	else
	{
		float4 diffuseColor = diffuseTexture[texPosition];
		float3 normal = float3(normalTexture[texPosition], 0.0f);
		normal.z = -sqrt(saturate(1.0f - sqr(normal.x) - sqr(normal.y)));

		return float4(normal, 1.0f);
	}
}