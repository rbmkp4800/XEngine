// NOTE: view-world space logic is completely broken
// TODO: fix that

struct DirectionalLight
{
	float4x4 shadowTextureTransform;
	float3 viewSpaceDirection;
	float3 color;
};

struct PointLight
{
	float4x4 shadowTextureTransforms[6];
	float3 viewSpacePosition;
	float3 color;
};

cbuffer Constants : register(b0)
{
	float4x4 inverseView;
	float ndcToViewDepthConversionA;
	float ndcToViewDepthConversionB;
	float aspect;
	float halfFOVTan;

	uint directionalLightCount;
	uint pointLightCount;

	DirectionalLight directionalLights[2];
	PointLight pointLights[4];
};

Texture2D<float4> albedoTexture : register(t0);
Texture2D<float4> normalRoughnessMetalnessTexture : register(t1);
Texture2D<float> depthTexture : register(t2);
Texture2D<float> directionalLightShadowMap : register(t3);
Texture2DArray<float> pointLightsShadowMaps : register(t4);

SamplerComparisonState shadowSampler : register(s0);

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

// https://github.com/KhronosGroup/glTF-WebGL-PBR
// http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

float DistributionGGX(float alphaRoughnessSqr, float NdotH)
{
	float f = (NdotH * alphaRoughnessSqr - NdotH) * NdotH + 1.0f;
	return alphaRoughnessSqr / (3.141592654f * sqr(f));
}

float GeometrySmithSchlick(float k, float NdotL, float NdotV)
{
	float l = NdotL * (1.0f - k) + k;
	float v = NdotV * (1.0f - k) + k;

	return rcp(v * l);
}

float3 FresnelSchlick(float3 f0, float NdotH)
{
	return f0 + (1.0f - f0) * pow(1.0f - NdotH, 5.0f);
}

float3 ComputeLighting(float3 diffuseColor, float3 specularColor, float3 lightColor,
	float3 V, float3 L, float3 N, float NdotV,
	float alphaRoughnessSqr, float k)
{
	float3 H = normalize(V + L);

	float NdotH = saturate(dot(N, H));
	float NdotL = saturate(dot(N, L));

	float D = DistributionGGX(alphaRoughnessSqr, NdotH);
	float G = GeometrySmithSchlick(k, NdotL, NdotV);
	float3 F = FresnelSchlick(specularColor, NdotH);

	float3 specularContribution = (D * G * F) * 0.25f; // NdotL * NdotV divistion is optimized
	float3 diffuseContribution = (1.0 - F) * diffuseColor;

	return (diffuseContribution + specularContribution) * lightColor * NdotL;
}

uint GetCubeFaceIndex(float3 v)
{
	// UnrealEngine/Engine/Shaders/Private/ShadowProjectionCommon.ush: CubemapHardwarePCF()

	float3 absv = abs(v);
	float maxCoordinate = max(absv.x, max(absv.y, absv.z));

	int cubeFaceIndex = 0;
	if (maxCoordinate == absv.x)
		cubeFaceIndex = absv.x == v.x ? 0 : 1;
	else if (maxCoordinate == absv.y)
		cubeFaceIndex = absv.y == v.y ? 2 : 3;
	else
		cubeFaceIndex = absv.z == v.z ? 4 : 5;

	return cubeFaceIndex;
}

float4 main(PSInput input) : SV_Target
{
	int2 texPosition = int2(input.position.xy);

	float ndcDepth = depthTexture[texPosition];
	if (ndcDepth == 1.0f)
		return float4(0.0f, 0.0f, 0.0f, 1.0f);

	float viewSpaceDepth = NDCDepthToViewSpaceDepth(ndcDepth);
	float3 viewSpacePosition = float3(input.ndcPosition * viewSpaceDepth * halfFOVTan, viewSpaceDepth);
	viewSpacePosition.x *= aspect;
	float3 V = -normalize(viewSpacePosition);

	float3 albedo = pow(albedoTexture[texPosition].xyz, 2.2f);
	float4 normalRoughnessMetalness = normalRoughnessMetalnessTexture[texPosition];
	float3 normal = float3(normalRoughnessMetalness.xy, 0.0f);
	normal.z = -sqrt(saturate(1.0f - sqr(normal.x) - sqr(normal.y)));
	float roughness = normalRoughnessMetalness.z;
	float metalness = normalRoughnessMetalness.w;

	float3 worldSpacePosition = mul((float3x4) inverseView, float4(viewSpacePosition, 1.0f));

	// Precompute values:
	//		for DistributionGGX:
	float alphaRoughnessSqr = sqr(sqr(roughness));
	//		for GeometrySmithSchlick:
	float k = sqr(roughness + 1.0f) / 8.0f;
	// float k = sqr(roughness) / 2.0f;
	// float k = sqr(roughness) * sqrt(2.0f / 3.141592654f);
	float NdotV = saturate(dot(normal, V));

	float3 diffuseColor = albedo * (1.0f - metalness) / 3.141592654f;
	float3 specularColor = lerp(0.04f, albedo, metalness);

	float3 sum = 0.0f;

	// directional lights
	[loop]
	for (uint i = 0; i < directionalLightCount; i++)
	{
		float3 shadowCoord = mul(directionalLights[i].shadowTextureTransform, float4(worldSpacePosition, 1.0f)).xyz;
		float shadow = directionalLightShadowMap.SampleCmpLevelZero(shadowSampler, shadowCoord.xy, shadowCoord.z - 0.01f);

		float3 lighting = ComputeLighting(diffuseColor, specularColor,
			directionalLights[i].color, V, directionalLights[i].viewSpaceDirection,
			normal, NdotV, alphaRoughnessSqr, k);

		sum += lighting * sqr(shadow);
	}

	// point lights
	[loop]
	for (uint i = 0; i < pointLightCount; i++) 
	{
		float3 lightWorldSpacePosition = mul((float3x4) inverseView, float4(pointLights[i].viewSpacePosition, 1.0f));
		float3 worldLightVector = worldSpacePosition - lightWorldSpacePosition;
		uint cubeFaceIndex = GetCubeFaceIndex(worldLightVector);

		float4 shadowCoord = mul(pointLights[i].shadowTextureTransforms[cubeFaceIndex], float4(worldSpacePosition, 1.0f));
		shadowCoord.xyz *= rcp(shadowCoord.w);

		float shadow = pointLightsShadowMaps.SampleCmpLevelZero(shadowSampler,
			float3(shadowCoord.xy, cubeFaceIndex), shadowCoord.z - 0.01f / shadowCoord.w);

		float3 lightVector = pointLights[i].viewSpacePosition - viewSpacePosition;
		float invLightDistance = rcp(length(lightVector));
		float3 L = lightVector * invLightDistance; // normalized light vector

		float3 lighting = ComputeLighting(diffuseColor, specularColor,
			pointLights[i].color * sqr(invLightDistance), V, L,
			normal, NdotV, alphaRoughnessSqr, k);

		sum += lighting * sqr(shadow);
	}

	float3 ambient = 0.03f * albedo;

	return float4(ambient + sum, 1.0f);
}