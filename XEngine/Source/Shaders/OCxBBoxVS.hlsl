cbuffer Constants : register(b0)
{
	float4x4 viewProjection;
};

StructuredBuffer<float4x3> transformsBuffer : register(t0);

static const uint cubeIndices[] =
{
	0, 1, 2,
	1, 3, 2,

	6, 5, 4,
	6, 7, 5,

	4, 1, 0,
	4, 5, 1,

	2, 3, 6,
	3, 7, 6,

	0, 2, 4,
	2, 6, 4,

	5, 3, 1,
	5, 7, 3,
};

struct VSOutput
{
	float4 position : SV_Position;
	nointerpolation uint geometryInstanceId : GEOMETRYINSTANCEID;
};

VSOutput main(uint id : SV_VertexID)
{
	uint geometryInstanceId = id / 36;

	uint cubeVertexId = cubeIndices[id % 36];
	float3 inputPosition;
	inputPosition.x = float((cubeVertexId >> 2) & 1);
	inputPosition.y = float((cubeVertexId >> 1) & 1);
	inputPosition.z = float(cubeVertexId & 1);
	inputPosition -= float3(0.5f, 0.5f, 0.5f);
	inputPosition *= 2.1f; // +0.1 for cube (when geometry = occlusion geometry
	float3 worldPosition = mul(float4(inputPosition, 1.0f), transformsBuffer[geometryInstanceId]);

	VSOutput output;
	output.position = mul(viewProjection, float4(worldPosition, 1.0f));
	output.geometryInstanceId = geometryInstanceId;
	return output;
}