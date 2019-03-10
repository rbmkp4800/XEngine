#include "GBufferNormal.hlsli"

struct Constants
{
	uint materialIndex;
};

struct Material
{
	uint albedoTextureIndex;
	float2 roughtnessMetalness;
};

struct MaterialsTable
{
	Material items[1024];
};

ConstantBuffer<Constants> constants : register(b0);
ConstantBuffer<MaterialsTable> materialsTable : register(b1);

Texture2D<float4> albedoTextures[16] : register(t0, space1);
SamplerState defaultSampler : register(s0);

struct PSInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
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
	float4 albedo = albedoTextures[albedoTextureIndex].Sample(defaultSampler, input.texcoord);

	PSOutput output;
	output.albedo = albedo;
	output.normalRoughnessMetalness.xy = EncodeNormal(normalize(input.normal));
	output.normalRoughnessMetalness.zw = float2(materialsTable.items[materialIndex].roughtnessMetalness);
	return output;
}