struct PSInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 color : COLOR;
};

struct PSOutput
{
	float4 diffuse : SV_Target0;
	float2 normal : SV_Target1;
};

PSOutput main(PSInput input)
{
	PSOutput output;
	output.diffuse = float4(input.color, 1.0f);
	output.normal = normalize(input.normal).xy;
	return output;
}