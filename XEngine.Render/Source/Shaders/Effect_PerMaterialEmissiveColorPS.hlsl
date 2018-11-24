struct Constants
{
	uint materialIndex;
};

struct Material
{
	float3 emissiveColor;
};

struct MaterialsTable
{
	Material items[1024];
};

ConstantBuffer<Constants> constants : register(b0);
ConstantBuffer<MaterialsTable> materialsTable : register(b1);

struct PSInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
};

struct PSOutput
{
	float4 albedo : SV_Target0;
	float4 normalRoughnessMetalness : SV_Target1;
	float3 luminance : SV_Target2;
};

PSOutput main(PSInput input)
{
	uint materialIndex = constants.materialIndex;

	PSOutput output;
	output.albedo = float4(0.0f, 0.0f, 0.0f, 1.0f);
	output.normalRoughnessMetalness.xy = normalize(input.normal).xy;
	output.normalRoughnessMetalness.zw = float2(1.0f, 0.0f);
	output.luminance = materialsTable.items[materialIndex].emissiveColor;
	return output;
}