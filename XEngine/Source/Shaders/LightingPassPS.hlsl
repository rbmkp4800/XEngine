static const float specularPower = 100.0f;

static const uint lightsLimit = 4;

struct Light
{
	float3 viewPosition;
	float3 color;
	float intensity;
};

cbuffer Constants : register(b0)
{
	float zNear, zFar, aspect, fovTg;
	Light lights[lightsLimit];
	uint lightsCount;
};

Texture2D<float4> diffuseTexture : register(t0);
Texture2D<float2> normalTexture : register(t1);
Texture2D<float> depthTexture : register(t2);

struct PSInput
{
	float4 position : SV_Position;
	float2 positionNDC : POSITION0;
};

float sqr(float value) { return value * value; }

float4 main(PSInput input) : SV_Target
{
	int3 texPosition = int3(input.position.xy, 0);

	float depthNDC = depthTexture.Load(texPosition);
	if (depthNDC == 1.0f) clip(-1.0f);
	float depth = (zFar * zNear) / (zFar - depthNDC * (zFar - zNear));

	float4 diffuseColor = diffuseTexture.Load(texPosition);
	float3 normal = float3(normalTexture.Load(texPosition), 0.0f);
	normal.z = -sqrt(saturate(1.0f - sqr(normal.x) - sqr(normal.y)));
	                       // ^^^ can be less then 0 (precision)

	//return float4(normal, 1.0f);
	//return float4(depth / 10.0f, depth / 10.0f, depth / 10.0f, 1.0f);

    float dd = 1.0f - depth / 20.0f;
    diffuseColor *= float4(dd, dd, dd, 1.0f);

	float3 viewPosition = float3(input.positionNDC * depth * fovTg, depth);
	viewPosition.x *= aspect;
	float3 normalizedViewPosition = normalize(viewPosition);

	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	float3 specular = float3(0.0f, 0.0f, 0.0f);
	for (uint i = 0; i < lightsCount; i++)
	{
		float3 lightVector = lights[i].viewPosition - viewPosition;
		float lightVectorLength = length(lightVector);
		lightVector /= lightVectorLength;	// normalize
		float distanceCoef = 1.0f - smoothstep(0.0f, lights[i].intensity, lightVectorLength);
		float normalCoef = saturate(dot(normal, lightVector));
		diffuse += lights[i].color * distanceCoef * normalCoef;

		float3 reflectVector = reflect(normalizedViewPosition, normal);
		float isFacingLightCoef = saturate(sign(dot(normal, lightVector)));
		specular += lights[i].color * lights[i].intensity *
			pow(saturate(dot(lightVector, reflectVector)) * isFacingLightCoef, specularPower);
	}

	return float4(diffuseColor.xyz * (0.3f + diffuse + specular), 1.0f);
}