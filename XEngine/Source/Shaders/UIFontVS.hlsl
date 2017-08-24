struct VSInput
{
	float2 position : POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 0.0f, 1.0f);
	output.texCoord = input.texCoord;
	output.color = input.color;

	return output;
}