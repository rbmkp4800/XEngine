#include <XLib.Color.h>
#include <XLib.Vectors.Math.h>
#include <XLib.Vectors.Arithmetics.h>
#include <XLib.Math.Matrix3x4.h>
#include <XEngine.Render.Device.h>
#include <XEngine.UI.TextRenderer.h>

#include "GameSample1.h"

using namespace XLib;
using namespace XEngine;
using namespace GameSample1;

static constexpr float32 cameraMouseSens = 0.005f;
static constexpr float32 cameraSpeed = 0.1f;

void Game::initialize()
{
	Core::Input::AddHandler(this);
	Core::Input::SetCursorState(Core::CursorState::Disabled);

	Render::Device& renderDevice = Core::Engine::GetRenderDevice();

	plainEffect = renderDevice.createEffect_perMaterialAlbedoRoughtnessMetalness();
	emissiveEffect = renderDevice.createEffect_perMaterialEmissiveColor();
	//plainMaterial = renderDevice.createMaterial(plainEffect);

	scene.initialize(renderDevice);
	gBuffer.initialize(renderDevice, { 1440, 900 });

	uiBatch.initialize(renderDevice);
	font.initializeDefault(renderDevice);

	cubeGeometryResource.createCube();
	sphereGeometryResource.createCubicSphere(6);

	for (uint32 i = 0; i < 8; i++)
	{
		for (uint32 j = 0; j < 8; j++)
		{
			Render::MaterialHandle mat = renderDevice.createMaterial(plainEffect);

			struct
			{
				float32x4 color;
				float32x4 roughtnessMetalness;
			} c;

			c.color = { 1.0f, 0.0f, 0.0f, 0.0f };
			c.roughtnessMetalness = { max(float32(i) / 8.0f, 0.05f), float32(j) / 8.0f, 0.0f, 0.0f };

			renderDevice.updateMaterialConstants(mat, 0, &c, sizeof(c));

			Render::TransformGroupHandle tgh = scene.createTransformGroup();
			Render::GeometryInstanceHandle inst = scene.createGeometryInstance(
				sphereGeometryResource.getGeometryDesc(), mat, tgh);

			scene.updateTransform(tgh, 0,
				Matrix3x4::Translation(i * 3.0f - 10.0f, j * 3.0f - 10.0f, 0.0f));
		}
	}

	Render::MaterialHandle mat = renderDevice.createMaterial(plainEffect);

	struct
	{
		float32x4 color;
		float32x4 roughtnessMetalness;
	} c;

	c.color = { 0.5f, 0.5f, 1.0f, 0.0f };
	c.roughtnessMetalness = { 0.7f, 0.7f, 0.0f, 0.0f };

	renderDevice.updateMaterialConstants(mat, 0, &c, sizeof(c));

	Render::TransformGroupHandle tgh = scene.createTransformGroup();
	Render::GeometryInstanceHandle inst = scene.createGeometryInstance(
		cubeGeometryResource.getGeometryDesc(), mat, tgh);
	scene.updateTransform(tgh, 0,
		Matrix3x4::Translation(3.0f, 3.0f, 0.0f) * Matrix3x4::Scale(3.0f, 3.0f, 3.0f));

	tgh = scene.createTransformGroup();
	inst = scene.createGeometryInstance(
		cubeGeometryResource.getGeometryDesc(), mat, tgh);
	scene.updateTransform(tgh, 0,
		Matrix3x4::Translation(3.0f, 3.0f, 0.0f) * Matrix3x4::Scale(0.5f, 0.5f, 10.0f));

	tgh = scene.createTransformGroup();
	inst = scene.createGeometryInstance(
		sphereGeometryResource.getGeometryDesc(), mat, tgh);
	scene.updateTransform(tgh, 0,
		Matrix3x4::Translation(-3.0f, -3.0f, -6.0f) * Matrix3x4::Scale(4.0f, 4.0f, 4.0f));



	mat = renderDevice.createMaterial(emissiveEffect);

	struct
	{
		float32x3 color;
	} e;

	e.color = { 5.0f, 5.0f, 50.0f };

	renderDevice.updateMaterialConstants(mat, 0, &e, sizeof(e));

	tgh = scene.createTransformGroup();
	inst = scene.createGeometryInstance(
		cubeGeometryResource.getGeometryDesc(), mat, tgh);
	scene.updateTransform(tgh, 0,
		Matrix3x4::Translation(3.0f, 3.0f, 5.0f) * Matrix3x4::Scale(0.3f, 0.3f, 15.0f));

	tgh = scene.createTransformGroup();
	inst = scene.createGeometryInstance(
		sphereGeometryResource.getGeometryDesc(), mat, tgh);
	scene.updateTransform(tgh, 0,
		Matrix3x4::Translation(10.0f, 10.0f, 10.0f) * Matrix3x4::Scale(0.3f, 0.3f, 0.3f));

	{
		Render::DirectionalLightDesc desc;
		desc.direction = { -1.0f, -1.0f, -1.0f };
		desc.color = { 10.0f, 10.0f, 10.0f };
		scene.createDirectionalLight(desc);
	}

	camera.position = { -13.0f, -7.0f, 10.0f };
	cameraRotation = { 0.0f, -0.7f };
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

	static float32 a = 0.0f;
	a += 0.01f;
	float32x3 dir(Math::Sin(a), Math::Cos(a), -1.0f);

	scene.updateDirectionalLightDirection(0, dir);

	Render::DebugOutput debugOutput = Render::DebugOutput::Disabled;
	if (Core::Input::IsKeyDown(VirtualKey('1')))
		debugOutput = Render::DebugOutput::Wireframe;

	renderDevice.renderScene(scene, camera, gBuffer, renderTarget,
		{ 0, 0, 1440, 900 }, false, debugOutput);

	uiBatch.beginDraw(renderTarget, { 0, 0, 1440, 900 });
	{
		UI::TextRenderer textRenderer;
		textRenderer.beginDraw(uiBatch);
		textRenderer.setPosition({ 10.0f, 10.0f });
		textRenderer.setFont(font);
		textRenderer.setColor(0xFFFF00_rgb);
		textRenderer.write("XEngine\nSample text");
		textRenderer.endDraw();
	}
	uiBatch.endDraw(true);

	renderDevice.renderUI(uiBatch);
}

void Game::onMouseMove(sint16x2 delta)
{
	cameraRotation.x += delta.x * 0.005f;
	cameraRotation.y -= delta.y * 0.005f;
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