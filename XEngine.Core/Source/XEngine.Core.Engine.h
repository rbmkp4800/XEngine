#pragma once

#include <XLib.Types.h>
#include <XLib.Vectors.h>

namespace XEngine::Core { class DiskWorker; }
namespace XEngine::Core { class UserIOManager; }
namespace XEngine::Render { class Device; }
namespace XEngine::Render { class Target; }

namespace XEngine::Core { class Engine; }

namespace XEngine::Core
{
	class GameBase abstract
	{
		friend Engine;

	protected:
		virtual void onInitialize() = 0;
		virtual void onUpdate(float32 timeDelta) = 0;
	};

	class Engine abstract final
	{
	public:
		static DiskWorker& GetDiskWorker();
		static UserIOManager& GetUserIOManager();
		static Render::Device& GetRenderDevice();

		static uint32 GetOutputViewCount();
		static Render::Target& GetOutputViewRenderTarget(uint32 outputViewIndex);
		static uint16x2 GetOutputViewRenderTargetSize(uint32 outputViewIndex);

		static void Run(GameBase* gameBase);
	};
}