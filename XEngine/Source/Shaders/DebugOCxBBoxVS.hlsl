cbuffer Constants : register(b0)
{
	float4x4 viewProjection;
};

struct BVHNode
{
	uint isLeafFlag_parentId;
	uint leftChildId_geometryInstanceId;
	uint rightChildId_drawCommandId;
	float3 bbox[2];
};

StructuredBuffer<BVHNode> bvhNodesBuffer : register(t0);

static const uint cubeIndices[] =
{
	0, 1,   0, 2,   1, 3,   2, 3,
	4, 5,   4, 6,   5, 7,   6, 7,
	0, 4,   1, 5,   2, 6,   3, 7,
};

float4 main(uint id: SV_VertexID) : SV_Position
{
	uint bvhNodeId = id / 24;
	uint cubeVertexId = cubeIndices[id % 24];
	float3 inputPosition;
	inputPosition.x = float(bvhNodesBuffer[bvhNodeId].bbox[cubeVertexId & 1].x);
	inputPosition.y = float(bvhNodesBuffer[bvhNodeId].bbox[(cubeVertexId >> 1) & 1].y);
	inputPosition.z = float(bvhNodesBuffer[bvhNodeId].bbox[cubeVertexId >> 2].z);
	return mul(viewProjection, float4(inputPosition, 1.0f));
}