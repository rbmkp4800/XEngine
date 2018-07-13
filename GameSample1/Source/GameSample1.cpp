#include <XLib.Math.Matrix3x4.h>
#include <XEngine.Render.Device.h>

#include "GameSample1.h"

using namespace XLib;
using namespace XEngine;
using namespace GameSample1;

void Game::initialize()
{
	Core::Input::AddHandler(this);

	Render::Device& renderDevice = Core::Engine::GetRenderDevice();

	plainEffect = renderDevice.createEffect_plain();
	plainMaterial = renderDevice.createMaterial(plainEffect);

	scene.initialize(renderDevice);
	gBuffer.initialize(renderDevice, { 1440, 900 });

	cubeGeometryResource.createCube();
	cubeGeometryInstance = scene.createGeometryInstance(
		cubeGeometryResource.getGeometryDesc(), plainMaterial);
	scene.updateGeometryInstanceTransform(cubeGeometryInstance, Matrix3x4::Identity());
}

void Game::update(float32 timeDelta)
{
	Render::Device& renderDevice = Core::Engine::GetRenderDevice();
	Render::Target& renderTarget = Core::Engine::GetOutputViewRenderTarget(0);

}

void Game::onKeyboard(XLib::VirtualKey key, bool state)
{
	if (key == XLib::VirtualKey::Escape && state)
		Core::Engine::Shutdown();
}

void Game::onCloseRequest()
{
	Core::Engine::Shutdown();
}