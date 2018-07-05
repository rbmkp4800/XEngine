#include <XLib.Math.Matrix3x4.h>

#include "GameSample1.h"

using namespace XLib;
using namespace GameSample1;

void Game::initialize()
{
	cubeGeometryResource.createCube();
	cubeGeometryInstance = scene.createGeometryInstance(
		cubeGeometryResource.getGeometryDesc(), 0);

	scene.updateTransform(cubeGeometryInstance, Matrix3x4::Identity());
}

void Game::update(float32 timeDelta)
{

}

void Game::onKeyboard(XLib::VirtualKey key, bool state)
{
	if (key == XLib::VirtualKey::Escape && state)
		XEngine::Core::Engine::Shutdown();
}

void Game::onCloseRequest()
{
	XEngine::Core::Engine::Shutdown();
}