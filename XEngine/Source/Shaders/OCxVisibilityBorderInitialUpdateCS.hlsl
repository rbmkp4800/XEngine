#define VISIBILITY_BORDER_NODE_LEAF_FLAG				0x80000000
#define VISIBILITY_BORDER_NODE_PREVIOUSLY_VISIBLE_FLAG	0x40000000

struct Constants
{
	uint visibilityBorderNodeCount;
};

//ConstantBuffer<Constants> constants : register(b0);
Buffer<uint> visibilityBorderNodesVisibility : register(t0);
RWBuffer<uint> visibilityBorderNodes : register(u0);

bool isLeafNode(uint id) { return id & VISIBILITY_BORDER_NODE_LEAF_FLAG; }
bool isPreviouslyVisibleNode(uint id) { return id & VISIBILITY_BORDER_NODE_PREVIOUSLY_VISIBLE_FLAG; }

[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	/*if (id.x >= constants.visibilityBorderNodeCount)
		return;

	uint nodeId = visibilityBorderNodes[id];
	uint isVisible = visibilityBorderNodesVisibility[id];

	if (isLeafNode(nodeId))
	{
		if (isPreviouslyVisibleNode(nodeId))
		{
			
		}
		else
		{

		}
	}
	else
	{
		if (isVisible)
		{

		}
	}*/
}