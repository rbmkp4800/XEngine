struct MaterialIndexBuffer
{
	uint index;
};

struct Material
{
	float3 albedo;
	float2 roughtnessMetalness;
};

struct MaterialsTable
{
	Material items[1024];
};

ConstantBuffer<MaterialIndexBuffer> materialIndexBuffer : register(b0);
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
};

PSOutput main(PSInput input)
{
	uint materialIndex = materialIndexBuffer.index;

	PSOutput output;
	output.albedo = float4(materialsTable.items[materialIndex].albedo, 1.0f);
	output.normalRoughnessMetalness.xy = normalize(input.normal).xy;
	output.normalRoughnessMetalness.zw = float2(materialsTable.items[materialIndex].roughtnessMetalness);
	return output;
}