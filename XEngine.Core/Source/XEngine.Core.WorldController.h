#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>

#include <XEngine.Render.Scene.h>

namespace XEngine::Core
{
	class WorldController : public XLib::NonCopyable
	{
	private:
		struct IndexTreeNode;

	private:
		Render::Scene *scene = nullptr;
		IndexTreeNode *indexTreeRoot = nullptr;

	public:
		WorldController() = default;
		~WorldController() = default;

		bool intialize(Render::Scene& scene, const char* filename);
		void destroy();

		void update(const float32x3& cameraPosition, float32 visibilityRange);
	};
}