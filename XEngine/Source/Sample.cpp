// TODO: remove
#include <stdio.h>

#include "Sample.h"

using namespace XLib;
using namespace XEngine;

void SampleWindow::onCreate(CreationArgs& args)
{
	xerDevice.initialize();
	xerWindowTarget.initialize(&xerDevice, this->getHandle(), args.width, args.height);

	xerSceneRender.initialize(&xerDevice);
	xerUIRender.initialize(&xerDevice);

	xerPlainEffect.initializePlain(&xerDevice);
	xerTexturedEffect.initializeTexture(&xerDevice);
	xerPlainSkinnedEffect.initializePlainSkinned(&xerDevice);
	XEResourceLoader::LoadDefaultFont(&xerDevice, &xerFont);

	xerScene.initialize(&xerDevice);
	xerConsole.initialize(&xerDevice, &xerFont);
	xerConsole.setCommandHandler(XEUIConsoleCommandHandler(this, &SampleWindow::onConsoleCommand));

	xerCamera.position = { 0.0f, 0.0f, 0.0f };
	xerCamera.fov = 1.0f;
	xerCameraRotation.angles = { 0.0f, 0.0f };

	width = args.width;
	height = args.height;

	xerScene.createLight(float32x3(3.0f, 0.0f, 0.0f), 0x0000FF_rgb, 4.0f);
	xerScene.createLight(float32x3(0.0f, 10.0f, 2.0f), 0xFF4040_rgb, 6.0f);
	xerScene.createLight(float32x3(8.0f, 11.0f, 10.0f), 0xFFFFFF_rgb, 30.0f);
	xerScene.createLight(float32x3(-10.0f, -11.0f, -10.0f), 0x00FFFF_rgb, 30.0f);
}

void SampleWindow::onResize(ResizingArgs& args)
{
	width = args.width;
	height = args.height;

	xerWindowTarget.resize(args.width, args.height);
}

void SampleWindow::onKeyboard(VirtualKey key, bool state)
{
	if (consoleEnabled)
		return;

	switch (key)
	{
	case VirtualKey('W'):	controls.forward = state;	break;
	case VirtualKey('A'):	controls.left = state;		break;
	case VirtualKey('S'):	controls.backward = state;	break;
	case VirtualKey('D'):	controls.right = state;		break;
	case VirtualKey('E'):	controls.up = state;		break;
	case VirtualKey('Q'):	controls.down = state;		break;
	case VirtualKey('1'):	controls.wireframe = state;	break;
	case VirtualKey('Z'):	controls.coefUp = state;	break;
	case VirtualKey('X'):	controls.coefDown = state;	break;

	case VirtualKey('B'):
		if (state)
			xerCamera.fov = 0.1f;
		else
			xerCamera.fov = 1.0f;
		break;

	case VirtualKey::Escape:
		if (state)
			this->destroy();
		break;

	case VirtualKey('O'):
		if (state)
			ocUpdatesEnabled = !ocUpdatesEnabled;
		break;
	}
}

void SampleWindow::onMouseButton(MouseState& mouseState, MouseButton button, bool state)
{
	if (button == MouseButton::Left)
	{
		captureMouseToCameraRotation = state;
		if (state)
			mouseLastPos = { mouseState.x, mouseState.y };
	}
}

void SampleWindow::onMouseMove(MouseState& state)
{
	if (captureMouseToCameraRotation)
	{
		xerCameraRotation.angles.x += float32(state.x - mouseLastPos.x) * 0.005f;
		xerCameraRotation.angles.y += float32(state.y - mouseLastPos.y) * 0.005f;
		mouseLastPos = { state.x, state.y };
	}
}

void SampleWindow::onCharacter(wchar character)
{
	if (consoleEnabled)
	{
		if (character == '`')
			consoleEnabled = false;
		else
			xerConsole.handleCharacter(character);
	}
	else if (character == '`')
		consoleEnabled = true;
}

void SampleWindow::update()
{
	float32x3 translation(0.0f, 0.0f, 0.0f);
	if (controls.forward)
		translation.z += 0.1f;
	if (controls.backward)
		translation.z -= 0.1f;

	if (controls.up)
		translation.y += 0.1f;
	if (controls.down)
		translation.y -= 0.1f;

	if (controls.right)
		translation.x += 0.1f;
	if (controls.left)
		translation.x -= 0.1f;

	xerCameraRotation.setCameraForwardAndTranslateRotated_cameraVertical(xerCamera, translation);

	XERTargetBuffer *xerTarget = xerWindowTarget.getCurrentTargetBuffer();

	XERSceneDrawTimings xerTimings = {};
	xerSceneRender.draw(xerTarget, &xerScene, xerCamera,
		controls.wireframe ? XERDebugWireframeMode::Enabled : XERDebugWireframeMode::Disabled,
		ocUpdatesEnabled, &xerTimings);

	// TODO: manage target buffer state transitions before and after UI rendering
	xerUIRender.beginDraw(xerTarget);
	{
		float32 totalFrameTime = xerTimings.lightingPassFinished;
		float32 objectPassTime = xerTimings.objectsPassFinished;
		float32 occlusionCullingTime = xerTimings.occlusionCullingFinished - xerTimings.objectsPassFinished;
		float32 occlusionCullingDownscaleTime = xerTimings.occlusionCullingDownscaleFinished - xerTimings.objectsPassFinished;
		float32 occlusionCullingBBoxDrawTime = xerTimings.occlusionCullingBBoxDrawFinished - xerTimings.occlusionCullingDownscaleFinished;
		float32 occlusionCullingCommandListUpdateTime = xerTimings.occlusionCullingFinished - xerTimings.occlusionCullingBBoxDrawFinished;
		float32 ligtingPassTime = xerTimings.lightingPassFinished - xerTimings.occlusionCullingFinished;

		totalFrameTime *= 1000.0f;
		objectPassTime *= 1000.0f;
		occlusionCullingTime *= 1000.0f;
		occlusionCullingDownscaleTime *= 1000.0f;
		occlusionCullingBBoxDrawTime *= 1000.0f;
		occlusionCullingCommandListUpdateTime *= 1000.0f;
		ligtingPassTime *= 1000.0f;

		char buffer[512];
		sprintf(buffer, "XEngine v0.0001 by RBMKP4800\nRunning %s\n"
			"Total frame time %5.2f ms\n"
			"OP %5.2f ms\n"
			"OC %5.2f ms (%5.2f %5.2f %5.2f)\n"
			"LP %5.2f ms\n"
			"OC updates %s\n"
			"Cam (%4.2f %4.2f %4.2f) -> (%4.2f %4.2f %4.2f)\n"
			"WORK IN PROGRESS",
			xerDevice.getName(), totalFrameTime, objectPassTime,
			occlusionCullingTime,
			occlusionCullingDownscaleTime, occlusionCullingBBoxDrawTime, occlusionCullingCommandListUpdateTime,
			ligtingPassTime,
			ocUpdatesEnabled ? "enabled" : "disabled",
			xerCamera.position.x, xerCamera.position.y, xerCamera.position.z,
			xerCamera.forward.x, xerCamera.forward.y, xerCamera.forward.z);

		xerUIRender.drawText(&xerFont, float32x2(10.0f, 10.0f), buffer, 0xFFFF00_rgb);

		float32 stackedBarChartValues[] =
		{
			objectPassTime,
			occlusionCullingDownscaleTime,
			occlusionCullingBBoxDrawTime,
			occlusionCullingCommandListUpdateTime,
			ligtingPassTime,
		};

		uint32 colors[] =
		{
			0x3BD3A0_rgb,
			0xE3B841_rgb,
			0xE5753B_rgb,
			0xE74343_rgb,
			0x0CA0D8_rgb,
		};

		xerUIRender.drawStackedBarChart(float32x2(10.0f, height - 40), 30.0f, 40.0f,
			stackedBarChartValues, colors, countof(stackedBarChartValues));
	}

	if (consoleEnabled)
		xerConsole.draw(&xerUIRender);

	xerUIRender.endDraw();
	xerWindowTarget.present(true);
}