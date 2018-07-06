struct VSOutput
{
	float4 position : SV_Position;
	float2 positionNDC : POSITION0;
};

VSOutput main(uint id : SV_VertexID)
{
	VSOutput output;
	output.position.x = (float)(id / 2) * 4.0f - 1.0f;
	output.position.y = (float)(id % 2) * 4.0f - 1.0f;
	output.position.z = 0.0f;
	output.position.w = 1.0f;
	output.positionNDC = output.position.xy;
	return output;
}