#include <XLib.Vectors.Math.h>
#include <XLib.Vectors.Arithmetics.h>
#include <XLib.Math.Matrix3x4.h>
#include <XEngine.Render.Device.h>

#include "GameSample1.h"

using namespace XLib;
using namespace XEngine;
using namespace GameSample1;

static constexpr float32 cameraMouseSens = 0.005f;
static constexpr float32 cameraSpeed = 0.1f;

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

	camera.position = { -10.0f, 0.0f, 0.0f };
	cameraRotation = { 0.0f, 0.0f };
}

void Game::update(float32 timeDelta)
{
	Render::Device& renderDevice = Core::Engine::GetRenderDevice();
	Render::Target& renderTarget = Core::Engine::GetOutputViewRenderTarget(0);

	{
		// TODO: refactor

		float32x3 viewSpaceTranslation = { 0.0f, 0.0f, 0.0f };
		float32 translationStep = cameraSpeed;
		if (Core::Input::IsKeyDown(VirtualKey('A')))
			viewSpaceTranslation.x -= translationStep;
		if (Core::Input::IsKeyDown(VirtualKey('D')))
			viewSpaceTranslation.x += translationStep;

		if (Core::Input::IsKeyDown(VirtualKey('Q')))
			viewSpaceTranslation.y -= translationStep;
		if (Core::Input::IsKeyDown(VirtualKey('E')))
			viewSpaceTranslation.y += translationStep;

		if (Core::Input::IsKeyDown(VirtualKey('W')))
			viewSpaceTranslation.z += translationStep;
		if (Core::Input::IsKeyDown(VirtualKey('S')))
			viewSpaceTranslation.z -= translationStep;

		float32x2 xyFroward = { 0.0f, 0.0f };
		float32x3 forward = VectorMath::SphericalCoords_zZenith_xReference(cameraRotation, xyFroward);
		float32x2 xyRight = VectorMath::NormalLeft(xyFroward); // TODO: fix

		camera.forward = forward;
		camera.position += forward * viewSpaceTranslation.z;
		camera.position.xy += xyRight * viewSpaceTranslation.x;
		camera.position.z += viewSpaceTranslation.y;
	}

	renderDevice.renderScene(scene, camera, gBuffer, renderTarget, { 0, 0, 1440, 900 });
}

void Game::onMouseMove(sint16x2 delta)
{
	if (Core::Input::IsMouseButtonDown(MouseButton::Left))
		cameraRotation += float32x2(delta) * 0.005f;
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