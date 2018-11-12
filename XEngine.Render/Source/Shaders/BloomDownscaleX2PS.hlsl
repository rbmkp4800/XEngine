Texture2D<float3> inputTexture : register(t0);
SamplerState defaultSampler : register(s0);

struct PSInput
{
	float4 position : SV_Position;
	// TODO: remove
	float2 ndcPosition : POSITION0;
	float2 texcoord : TEXCOORD0;
};

float3 main(PSInput input) : SV_Target
{
	return inputTexture.SampleLevel(defaultSampler, input.texcoord, 0.0f);
}