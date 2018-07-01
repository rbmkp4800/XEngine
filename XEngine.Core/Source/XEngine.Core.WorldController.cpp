#include "XEngine.Core.WorldController.h"

#include "XEngine.Core.GeometryResource.h"

using namespace XEngine::Core;

struct WorldController::IndexTreeNode
{
	IndexTreeNode *parent;

	float32x3 position;
	float32 lodInreaseDistance;
	uint8 childrenCount;

	inline IndexTreeNode* getChildren() { return this + 1; }
};

void WorldController::update(const float32x3& cameraPosition, float32 visibilityRange)
{
	
}