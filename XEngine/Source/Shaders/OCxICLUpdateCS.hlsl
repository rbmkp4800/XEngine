struct DefaultDrawingIndirectCommand
{
	uint4 vbv;
	uint4 ibv;

	uint textureId;

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
	uint commandListSize;
};

ConstantBuffer<Constants> constants : register(b0);
StructuredBuffer<uint> geometryInstancesVisibility : register(t0);
RWStructuredBuffer<DefaultDrawingIndirectCommand> commandList : register(u0);
AppendStructuredBuffer<DefaultDrawingIndirectCommand> missedDrawCommandList : register(u1);

[numthreads(1024, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= constants.commandListSize)
		return;

	uint geometryInstanceId = commandList[id.x].geometryInstanceId;
	uint prevVisibilityValue = commandList[id.x].instanceCount;
	uint currentVisibilityValue = geometryInstancesVisibility[geometryInstanceId];
	commandList[id.x].instanceCount = currentVisibilityValue;

	if (prevVisibilityValue == 0 && currentVisibilityValue == 1)
	{
		DefaultDrawingIndirectCommand command = commandList[id.x];
        missedDrawCommandList.Append(command);
    }
}