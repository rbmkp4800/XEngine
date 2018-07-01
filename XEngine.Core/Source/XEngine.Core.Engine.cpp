#include <XEngine.Render.Device.h>
#include <XEngine.Render.Target.h>

#include "XEngine.Core.Engine.h"

#include "XEngine.Core.DiskWorker.h"
#include "XEngine.Core.Input.h"
#include "XEngine.Core.Output.h"

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Core;

static DiskWorker diskWorker;
static Render::Device renderDevice;

static Render::SwapChain outputSwapChain;

void Engine::Run(GameBase* gameBase)
{
	Output::Initialize();
	renderDevice.initialize();
	//diskWorker.initialize(0x40000);

	gameBase->initialize();

	outputSwapChain.initialize(renderDevice,
		Output::GetViewWindowHandle(0), Output::GetViewResolution(0));

	for (;;)
	{
		if (outputSwapChain.getSize() != Output::GetViewResolution(0))
			outputSwapChain.resize(Output::GetViewResolution(0));

		gameBase->update(0.0f);

		outputSwapChain.present();
	}
}

DiskWorker& Engine::GetDiskWorker() { return diskWorker; }
Render::Device& Engine::GetRenderDevice() { return renderDevice; }

uint32 Engine::GetOutputViewCount() { return 1; }
Render::Target& Engine::GetOutputViewRenderTarget(uint32 outputViewIndex) { return outputSwapChain.getCurrentTarget(); }
uint16x2 Engine::GetOutputViewRenderTargetSize(uint32 outputViewIndex) { return outputSwapChain.getSize(); }