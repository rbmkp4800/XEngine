Texture2D<float3> hdrTexture : register(t0);

cbuffer Constants : register(b0)
{
	float exposure;
};

float4 main(float4 position : SV_Position) : SV_Target
{
	int2 texPosition = int2(position.xy);

	float3 luminance = hdrTexture[texPosition];
	float3 mappedColor = float3(1.0f, 1.0f, 1.0f) - exp(-luminance * exposure);
	return float4(pow(mappedColor, 1.0f / 2.2f), 1.0f);
}