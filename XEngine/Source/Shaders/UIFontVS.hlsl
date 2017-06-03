struct VSInput
{
	float2 position : POSITION;
	float2 texCoord : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 0.0f, 1.0f);
	output.texCoord = input.texCoord;

	output.position.xy /= float2(1440.0f / 2.0f, 900.0f / 2.0f);
	output.position.xy -= float2(1.0f, 1.0f);
	output.position.y *= -1.0f;

	return output;
}