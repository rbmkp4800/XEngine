static const float specularPower = 10.0f;
static const float glareRadiusScale = 0.5f;
static const float glareIntensityScale = 0.01f;

static const uint lightsLimit = 4;

struct Light
{
	float3 vsPosition;
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

// vs - view space
float ComputePixelGlareIntensity(float3 vsLightPosition, float glareRadius, float3 vsPixelPosition)
{
    float viewerToLightDistance = length(vsLightPosition);
    float3 normalizedVSLightPosition = vsLightPosition / viewerToLightDistance;

    // alpha - angle between light direction ray and pixel direction ray
    float cosAlpha = dot(normalizedVSLightPosition, normalize(vsPixelPosition));
    float sinAlpha = sqrt(saturate(1.0f - sqr(cosAlpha)));

    float viewerToGlareSphereIntersectionCenterDistance = cosAlpha * viewerToLightDistance;
    float lightToGlareSphereIntersectionCenterDistance = sinAlpha * viewerToLightDistance;

    float glareSphereIntersectionHalfLength = sqrt(max(sqr(glareRadius) -
        sqr(lightToGlareSphereIntersectionCenterDistance), 0.0f));

    float intersectionNear = viewerToGlareSphereIntersectionCenterDistance - glareSphereIntersectionHalfLength;
    float intersectionFar = viewerToGlareSphereIntersectionCenterDistance + glareSphereIntersectionHalfLength;

    if (intersectionNear > vsPixelPosition.z)
        return 0.0f;
    if (intersectionFar < zNear)
        return 0.0f;

    intersectionNear = max(intersectionNear, zNear);
    intersectionFar = min(intersectionFar, vsPixelPosition.z);
    intersectionNear -= viewerToGlareSphereIntersectionCenterDistance;
    intersectionFar -= viewerToGlareSphereIntersectionCenterDistance;

    // pixel glare intensity equals integral of the light scattering at each point,
    // where scattering inversely proportional to the square of the distance to light
    //    integrate dx / sqr(lightToGlareSphereIntersectionCenterDistance) + sqr(x) from near to far
    float inverse = 1.0f / lightToGlareSphereIntersectionCenterDistance;
    float intensity = inverse * (atan(intersectionFar * inverse) - atan(intersectionNear * inverse));

    // remove step on the edge of glare circle
    intensity *= saturate(1.0f - lightToGlareSphereIntersectionCenterDistance / glareRadius);
    return intensity * glareRadius;
}

float4 main(PSInput input) : SV_Target
{
	int2 texPosition = int2(input.position.xy);

	float depthNDC = depthTexture[texPosition];

    float depth = (zFar * zNear) / (zFar - depthNDC * (zFar - zNear));
    float3 vsPixelPosition = float3(input.positionNDC * depth * fovTg, depth);
    vsPixelPosition.x *= aspect;
    float3 normalizedVSPixelPosition = normalize(vsPixelPosition);

    if (depthNDC == 1.0f)
    {
        float3 glare = float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < lightsCount; i++)
        {
            float glareIntensity = ComputePixelGlareIntensity(lights[i].vsPosition,
                lights[i].intensity * glareRadiusScale, vsPixelPosition);
            glare += lights[i].color * glareIntensity * glareIntensityScale;
        }

        return float4(glare, 1.0f);
    }
    else
    {
        float4 diffuseColor = diffuseTexture[texPosition];
        float3 normal = float3(normalTexture[texPosition], 0.0f);
        normal.z = -sqrt(saturate(1.0f - sqr(normal.x) - sqr(normal.y)));
	                       // ^^^ can be less then 0 (precision)

        float3 diffuse = float3(0.0f, 0.0f, 0.0f);
        float3 specular = float3(0.0f, 0.0f, 0.0f);
        float3 glare = float3(0.0f, 0.0f, 0.0f);

        for (uint i = 0; i < lightsCount; i++)
        {
            float3 pixelToLightVector = lights[i].vsPosition - vsPixelPosition;
            float pixelToLightDistance = length(pixelToLightVector);
            float3 normalizedPixelToLightVector = pixelToLightVector / pixelToLightDistance;

            {
                float distanceCoef = 1.0f - smoothstep(0.0f, lights[i].intensity, pixelToLightDistance);
                float normalCoef = saturate(dot(normal, normalizedPixelToLightVector));
                diffuse += lights[i].color * distanceCoef * normalCoef;
            }

            {
                float3 reflectVector = reflect(normalizedVSPixelPosition, normal);
                float isFacingLightCoef = saturate(sign(dot(normal, normalizedPixelToLightVector)));
                specular += lights[i].color * lights[i].intensity * 0.03f *
			    pow(saturate(dot(normalizedPixelToLightVector, reflectVector)) * isFacingLightCoef, specularPower);
            }

            {
                float glareIntensity = ComputePixelGlareIntensity(lights[i].vsPosition,
                lights[i].intensity * glareRadiusScale, vsPixelPosition);
                glare += lights[i].color * glareIntensity * glareIntensityScale;
            }
        }

        return float4(diffuseColor.xyz * (diffuse + specular) + glare, 1.0f);
    }
}