Texture2D<float3> inputTexture : register(t0);
SamplerState defaultSampler : register(s0);

cbuffer Constants : register(b0)
{
	float2 inverseInputSize;
}

struct PSInput
{
	float4 position : SV_Position;
	float2 ndcPosition : POSITION0;
	float2 texcoord : TEXCOORD0;
};

float SRGBToLuminance(float3 srgb)
{
	return dot(srgb, float3(0.212671f, 0.71516f, 0.07217f));
}

float3 Q(float3 a)
{
	const float luminance0 = SRGBToLuminance(a);

	a *= max(0.0f, luminance0 - 1.8f) / (luminance0 + 0.001f);

	return a;
}

float3 main(PSInput input) : SV_Target
{
	/*float2 offset = inverseInputSize.xy * 0.25f;

	float3 sample0 = inputTexture.SampleLevel(defaultSampler, input.texcoord + float2(-offset.x, -offset.y), 0.0f);
	float3 sample1 = inputTexture.SampleLevel(defaultSampler, input.texcoord + float2( offset.x, -offset.y), 0.0f);
	float3 sample2 = inputTexture.SampleLevel(defaultSampler, input.texcoord + float2(-offset.x,  offset.y), 0.0f);
	float3 sample3 = inputTexture.SampleLevel(defaultSampler, input.texcoord + float2( offset.x,  offset.y), 0.0f);

	const float luminance0 = SRGBToLuminance(sample0);
	const float luminance1 = SRGBToLuminance(sample1);
	const float luminance2 = SRGBToLuminance(sample2);
	const float luminance3 = SRGBToLuminance(sample3);

	const float threshold = 0.1f;

	sample0 *= max(0.0f, luminance0 - threshold);
	sample1 *= max(0.0f, luminance1 - threshold);
	sample2 *= max(0.0f, luminance2 - threshold);
	sample3 *= max(0.0f, luminance3 - threshold);

	return sample0 + sample1 + sample2 + sample3;*/

	int2 texPosition = int2(input.position.xy) * 4;

    float3 s00 = inputTexture[texPosition + int2(0, 0)];
    float3 s01 = inputTexture[texPosition + int2(0, 1)];
    float3 s02 = inputTexture[texPosition + int2(0, 2)];
    float3 s03 = inputTexture[texPosition + int2(0, 3)];

    float3 s10 = inputTexture[texPosition + int2(1, 0)];
    float3 s11 = inputTexture[texPosition + int2(1, 1)];
    float3 s12 = inputTexture[texPosition + int2(1, 2)];
    float3 s13 = inputTexture[texPosition + int2(1, 3)];

	float3 s20 = inputTexture[texPosition + int2(2, 0)];
    float3 s21 = inputTexture[texPosition + int2(2, 1)];
    float3 s22 = inputTexture[texPosition + int2(2, 2)];
    float3 s23 = inputTexture[texPosition + int2(2, 3)];

	float3 s30 = inputTexture[texPosition + int2(3, 0)];
    float3 s31 = inputTexture[texPosition + int2(3, 1)];
    float3 s32 = inputTexture[texPosition + int2(3, 2)];
    float3 s33 = inputTexture[texPosition + int2(3, 3)];

	s00 = Q(s00);
	s01 = Q(s01);
	s02 = Q(s02);
	s03 = Q(s03);

	s10 = Q(s10);
	s11 = Q(s11);
	s12 = Q(s12);
	s13 = Q(s13);

	s20 = Q(s20);
	s21 = Q(s21);
	s22 = Q(s22);
	s23 = Q(s23);

	s30 = Q(s30);
	s31 = Q(s31);
	s32 = Q(s32);
    s33 = Q(s33);

	return s00 + s01 + s02 + s03
		+ s10 + s11 + s12 + s13
		+ s20 + s21 + s22 + s23
		+ s30 + s31 + s32 + s33;
}