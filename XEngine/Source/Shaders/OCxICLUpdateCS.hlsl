struct DefaultDrawingIndirectCommand
{
	uint4 vbv;
	uint4 ibv;

	// D3D12_DRAW_INDEXED_ARGUMENTS
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int baseVertexLocation;
	uint startInstanceLocation;

	uint geometryInstanceId;
};

struct Constants
{
	uint iclSize;
};

ConstantBuffer<Constants> constants : register(b0);
StructuredBuffer<uint> geometryInstancesVisibility : register(t0);
RWStructuredBuffer<DefaultDrawingIndirectCommand> drawingICL : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= constants.iclSize)
		return;

	uint geometryInstanceId = drawingICL[id.x].geometryInstanceId;
	drawingICL[id.x].instanceCount = geometryInstancesVisibility[geometryInstanceId];
}