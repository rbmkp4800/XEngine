static const float specularPower = 100.0f;
static const float abmientIntensity = 0.3f;
static const float glareRadiusScale = 0.5f;
static const float glareIntensityScale = 0.01f;

static const uint lightsLimit = 4;

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
	float fovTg;
	Light lights[lightsLimit];
	uint lightsCount;
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

float ComputePixelGlareIntensity(float3 viewSapceLightPosition, float glareRadius, float3 viewSpacePosition)
{
    float viewerToLightDistance = length(viewSapceLightPosition);
    float3 normalizedViewSapceLightPosition = viewSapceLightPosition / viewerToLightDistance;

    // alpha - angle between light direction ray and pixel direction ray
    float cosAlpha = dot(normalizedViewSapceLightPosition, normalize(viewSpacePosition));
    float sinAlpha = sqrt(saturate(1.0f - sqr(cosAlpha)));

    float viewerToGlareSphereIntersectionCenterDistance = cosAlpha * viewerToLightDistance;
    float lightToGlareSphereIntersectionCenterDistance = sinAlpha * viewerToLightDistance;

    float glareSphereIntersectionHalfLength = sqrt(max(sqr(glareRadius) -
        sqr(lightToGlareSphereIntersectionCenterDistance), 0.0f));

    float intersectionNear = viewerToGlareSphereIntersectionCenterDistance - glareSphereIntersectionHalfLength;
    float intersectionFar = viewerToGlareSphereIntersectionCenterDistance + glareSphereIntersectionHalfLength;

	float result = 0.0f;
	if (intersectionNear <= viewSpacePosition.z && intersectionFar > 0.0f)
	{
		intersectionNear = max(intersectionNear, 0.0f);
		intersectionFar = min(intersectionFar, viewSpacePosition.z);
		intersectionNear -= viewerToGlareSphereIntersectionCenterDistance;
		intersectionFar -= viewerToGlareSphereIntersectionCenterDistance;

		// pixel glare intensity equals integral of the light scattering at each point,
		// where scattering inversely proportional to the square of the distance to light
		//    integrate dx / sqr(lightToGlareSphereIntersectionCenterDistance) + sqr(x) from near to far
		float inverse = 1.0f / lightToGlareSphereIntersectionCenterDistance;
		float intensity = inverse * (atan(intersectionFar * inverse) - atan(intersectionNear * inverse));

		// remove step on the edge of glare circle
		intensity *= saturate(1.0f - lightToGlareSphereIntersectionCenterDistance / glareRadius);
		result = intensity * glareRadius;
	}

	return result;
}

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
    float3 viewSpacePosition = float3(input.ndcPosition * viewSpaceDepth * fovTg, viewSpaceDepth);
	viewSpacePosition.x *= aspect;
    float3 normalizedViewSpacePosition = normalize(viewSpacePosition);

    if (ndcDepth == 1.0f)
    {
        float3 glare = float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < lightsCount; i++)
        {
            float glareIntensity = ComputePixelGlareIntensity(lights[i].viewSpacePosition,
                lights[i].intensity * glareRadiusScale, viewSpacePosition);
            glare += lights[i].color * glareIntensity * glareIntensityScale;
        }

        return float4(glare, 1.0f);
    }
    else
    {
        float4 diffuseColor = diffuseTexture[texPosition];
        float3 normal = float3(normalTexture[texPosition], 0.0f);
        normal.z = -sqrt(saturate(1.0f - sqr(normal.x) - sqr(normal.y)));
	                           // ^^^ can be less then 0 (precision issue)

        float3 diffuse = float3(0.0f, 0.0f, 0.0f);
        float3 specular = float3(0.0f, 0.0f, 0.0f);
        float3 glare = float3(0.0f, 0.0f, 0.0f);

        for (uint i = 0; i < lightsCount; i++)
        {
            float3 pixelToLightVector = lights[i].viewSpacePosition - viewSpacePosition;
            float pixelToLightDistance = length(pixelToLightVector);
            float3 normalizedPixelToLightVector = pixelToLightVector / pixelToLightDistance;

            {
                float distanceCoef = 1.0f - smoothstep(0.0f, lights[i].intensity, pixelToLightDistance);
                float normalCoef = saturate(dot(normal, normalizedPixelToLightVector));
                diffuse += lights[i].color * distanceCoef * normalCoef;
            }

            {
                float3 reflectVector = reflect(normalizedViewSpacePosition, normal);
                float isFacingLightCoef = saturate(sign(dot(normal, normalizedPixelToLightVector)));
                specular += lights[i].color * lights[i].intensity * 0.03f *
					pow(saturate(dot(normalizedPixelToLightVector, reflectVector)) * isFacingLightCoef, specularPower);
            }

            {
                float glareIntensity = ComputePixelGlareIntensity(lights[i].viewSpacePosition,
                lights[i].intensity * glareRadiusScale, viewSpacePosition);
                glare += lights[i].color * glareIntensity * glareIntensityScale;
            }
        }

        return float4(diffuseColor.xyz * (abmientIntensity + diffuse + specular) + glare, 1.0f);
    }
}