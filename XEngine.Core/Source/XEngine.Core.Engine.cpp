#include <XEngine.Render.Device.h>
#include <XEngine.Render.Target.h>

#include "XEngine.Core.Engine.h"

#include "XEngine.Core.DiskWorker.h"
#include "XEngine.Core.UserIOManager.h"

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Core;

namespace
{
	DiskWorker diskWorker;
	UserIOManager userIOManager;
	Render::Device renderDevice;

	Render::SwapChain outputSwapChain;
}

void Engine::Run(GameBase* gameBase)
{
	userIOManager.initialize();
	renderDevice.initialize();
	diskWorker.initialize(0x10000);

	gameBase->onInitialize();

	outputSwapChain.initialize(renderDevice, userIOManager.getOutputViewWindowHandle(0),
		userIOManager.getOutputViewResolution(0));

	for (;;)
	{
		if (outputSwapChain.getSize() != userIOManager.getOutputViewResolution(0))
			outputSwapChain.resize(userIOManager.getOutputViewResolution(0));

		userIOManager.update();
		gameBase->onUpdate(0.0f);
	}
}

DiskWorker& Engine::GetDiskWorker() { return diskWorker; }
UserIOManager& Engine::GetUserIOManager() { return userIOManager; }
Render::Device& Engine::GetRenderDevice() { return renderDevice; }

uint32 Engine::GetOutputViewCount() { return 1; }
Render::Target& Engine::GetOutputViewRenderTarget(uint32 outputViewIndex) { return outputSwapChain.getCurrentTarget(); }
uint16x2 Engine::GetOutputViewRenderTargetSize(uint32 outputViewIndex) { return outputSwapChain.getSize(); }