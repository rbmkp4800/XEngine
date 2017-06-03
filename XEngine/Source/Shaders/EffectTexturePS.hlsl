Texture2D<float4> diffuseTexture : register(t0);
SamplerState defaultSampler : register(s0);

struct PSInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
};

struct PSOutput
{
	float4 diffuse : SV_Target0;
	float2 normal : SV_Target1;
};

PSOutput main(PSInput input)
{
	PSOutput output;
	int2 it = input.tex * 10.0f;
	/*if ((it.x + it.y) % 2)
		output.diffuse = float4(1.0f, 1.0f, 1.0f, 0.1f);
	else
		output.diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);*/

	output.diffuse = diffuseTexture.Sample(defaultSampler, input.tex);
	output.normal = normalize(input.normal).xy;
	return output;
}