Texture2D<float3> inputTexture : register(t0);
SamplerState defaultSampler : register(s0);

cbuffer Constants : register(b0)
{
	float2 step;
	float inputScale;
}

struct PSInput
{
	float4 position : SV_Position;
	// TODO: remove
	float2 ndcPosition : POSITION0;
	float2 texcoord : TEXCOORD0;
};

static const float weight0 = 0.227027029f;
static const float weight1 = 0.316216230f;
static const float weight2 = 0.0702702701f;

static const float offset1 = 1.38461530f;
static const float offset2 = 3.23076916f;

float3 main(PSInput input) : SV_Target
{
	const float3 sample0 = inputTexture.SampleLevel(defaultSampler, inputScale * input.texcoord, 0.0f);

	const float3 sampleP1 = inputTexture.SampleLevel(defaultSampler, inputScale * (input.texcoord + step * offset1), 0.0f);
	const float3 sampleP2 = inputTexture.SampleLevel(defaultSampler, inputScale * (input.texcoord + step * offset2), 0.0f);
	const float3 sampleN1 = inputTexture.SampleLevel(defaultSampler, inputScale * (input.texcoord - step * offset1), 0.0f);
	const float3 sampleN2 = inputTexture.SampleLevel(defaultSampler, inputScale * (input.texcoord - step * offset2), 0.0f);

	return sample0 * weight0 + (sampleP1 + sampleN1) * weight1 + (sampleP2 + sampleN2) * weight2;
}