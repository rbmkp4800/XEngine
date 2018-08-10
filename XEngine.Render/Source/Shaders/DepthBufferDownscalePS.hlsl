Texture2D<float> sourceTexture : register(t0, space1);

// TODO: check gather performance

float main(float4 position : SV_Position) : SV_Target
{
	int2 texPosition = int2(position.xy) * 2;

	float result = 0.0f;
	[unroll]
	for (int i = 0; i < 2; i++)
	{
		[unroll]
		for (int j = 0; j < 2; j++)
			result = max(result, sourceTexture[texPosition + int2(j, i)]);
	}

	return result;
}