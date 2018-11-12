Texture2D<float3> hdrTexture : register(t0);
Texture2D<float3> bloomTextures[4] : register(t1);
SamplerState defaultSampler : register(s0);

cbuffer Constants : register(b0)
{
	float exposure;
};

struct PSInput
{
	float4 position : SV_Position;
	// TODO: remove
	float2 ndcPosition : POSITION0;
	float2 texcoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target
{
	int2 texPosition = int2(input.position.xy);

	float3 luminance = hdrTexture[texPosition];
	//float3 luminance = float3(0.0, 0.0, 0.0);

	luminance += bloomTextures[0].SampleLevel(defaultSampler, input.texcoord, 0.0f) * 0.125f * 0.8f;
	luminance += bloomTextures[1].SampleLevel(defaultSampler, input.texcoord, 0.0f) * 0.125f * 0.4f;
	luminance += bloomTextures[2].SampleLevel(defaultSampler, input.texcoord, 0.0f) * 0.125f * 0.2f;
	luminance += bloomTextures[3].SampleLevel(defaultSampler, input.texcoord, 0.0f) * 0.125f * 0.1f;

	float3 mappedColor = float3(1.0f, 1.0f, 1.0f) - exp(-luminance * exposure);
	return float4(pow(mappedColor, 1.0f / 2.2f), 1.0f);
}