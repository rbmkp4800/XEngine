struct Constants
{
	uint materialIndex;
};

struct Material
{
	uint albedoTextureIndex;
	uint normalRoughnessMetalnessTextureIndex;
};

struct MaterialsTable
{
	Material items[1024];
};

ConstantBuffer<Constants> constants : register(b0);
ConstantBuffer<MaterialsTable> materialsTable : register(b1);

Texture2D<float4> textures[16] : register(t0, space1);
SamplerState defaultSampler : register(s0);

struct PSInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	float2 texcoord : TEXCOORD;
};

struct PSOutput
{
	float4 albedo : SV_Target0;
	float4 normalRoughnessMetalness : SV_Target1;
};

PSOutput main(PSInput input)
{
	uint materialIndex = constants.materialIndex;
	uint albedoTextureIndex = materialsTable.items[materialIndex].albedoTextureIndex;
	uint normalRoughnessMetalnessTextureIndex = materialsTable.items[materialIndex].normalRoughnessMetalnessTextureIndex;
	float4 albedo = textures[albedoTextureIndex].Sample(defaultSampler, input.texcoord);
	float4 normalRoughnessMetalness = textures[normalRoughnessMetalnessTextureIndex].Sample(defaultSampler, input.texcoord);

	float3x3 TBN = transpose(float3x3(input.tangent, input.bitangent, input.normal));

	float2 normalSample = normalRoughnessMetalness.xy * 2.0f - 1.0f;
	float3 tangentSpaceNormal = float3(normalSample, 0.0f);
	tangentSpaceNormal.z = sqrt(saturate(1.0f - dot(normalSample, normalSample)));

	float3 normal = normalize(mul(TBN, tangentSpaceNormal));

	PSOutput output;
	output.albedo = albedo;
	output.normalRoughnessMetalness.xy = normal.xy;
	output.normalRoughnessMetalness.zw = normalRoughnessMetalness.zw;
	return output;
}