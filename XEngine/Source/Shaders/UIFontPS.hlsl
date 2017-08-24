Texture2D<float4> defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

struct PSInput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

float4 main(PSInput input) : SV_Target
{
	float alpha = defaultTexture.Sample(defaultSampler, input.texCoord).w;
	alpha *= input.color.w;
	if (alpha < 0.01f)
		clip(-1.0f);
	return float4(input.color.xyz, alpha);
}