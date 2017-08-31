// TODO: remove
#include <stdio.h>

#include <XLib.Program.h>
#include <XLib.System.Window.h>
#include <XLib.System.Timer.h>
#include <XLib.Random.h>

#include "XEngine.h"

using namespace XLib;
using namespace XEngine;

class Window : public WindowBase
{
private:
	XERDevice xerDevice;
	XERWindowTarget xerWindowTarget;

	XERMonospacedFont xerFont;
	XERSceneRender xerSceneRender;
	XERUIRender xerUIRender;

	XEUIConsole xerConsole;

	XERScene xerScene;
	XEREffect xerPlainEffect;
	XEREffect xerTexturedEffect;
	XEREffect xerPlainSkinnedEffect;

	XERGeometry xerPlaneGeometry;
	XERGeometry xerCubeGeometry;
	//XERGeometry xerMonsterGeometry;
	XERGeometry xerMonsterSkinnedGeometry;
	XERGeometry xerSphereGeometry;
	XERTexture xerDefaultTexture;

	XERGeometryInstanceId cubesCircleIds[10];

	XERGeometryInstanceId planeGeometryInstanceId;
	XERGeometryInstanceId cubeGeometryInstanceId;
	XERGeometryInstanceId monsterGeometryInstanceId;
	XERLightId light1Id, light2Id, light3Id, light4Id;

	XERCamera xerCamera;
	XERCameraRotation xerCameraRotation;
	sint16x2 mouseLastPos = { 0, 0 };
	bool captureMouseToCameraRotation = false;

	struct { bool left, right, up, down, forward, backward, wireframe, narrowFOV, coefUp, coefDown; } controls = {};
	bool ocUpdatesEnabled = false;

	float32 time = 0.0f;
	float32 coef = 0.01f;

	float frameTimeSum = 0.0f;
	uint32 frameCount = 0;
	TimerRecord lastFrameTimerRecord;

	bool consoleEnabled = false;

	virtual void onCreate(CreationArgs& args) override
	{
		xerDevice.initialize();
		xerSceneRender.initialize(&xerDevice);
		xerWindowTarget.initialize(&xerDevice, this->getHandle(), args.width, args.height);

		XEResourceLoader::LoadDefaultFont(&xerDevice, &xerFont);
		xerUIRender.initialize(&xerDevice);
		xerConsole.initialize(&xerDevice, &xerFont);
		
		xerScene.initialize(&xerDevice);
		xerPlainEffect.initializePlain(&xerDevice);
		xerTexturedEffect.initializeTexture(&xerDevice);
		xerPlainSkinnedEffect.initializePlainSkinned(&xerDevice);

		XERGeometryGenerator::HorizontalPlane(&xerDevice, &xerPlaneGeometry);
		XERGeometryGenerator::Cube(&xerDevice, &xerCubeGeometry);
		//XERGeometryGenerator::Monster(&xerDevice, &xerMonsterGeometry);
		XERGeometryGenerator::MonsterSkinned(&xerDevice, &xerMonsterSkinnedGeometry);
		XERGeometryGenerator::Sphere(4, &xerDevice, &xerSphereGeometry);

		{
			static constexpr uint32 n = 1024;
			static uint32 tex[n][n];
			for (uint32 i = 0; i < n; i++)
			{
				for (uint32 j = 0; j < n; j++)
				{
					uint32 v = (i ^ j);
					tex[i][j] = v << 16 | v << 8 | v | 0x000000FF_rgba;
				}
			}

			xerDefaultTexture.initialize(&xerDevice, tex, n, n);
		}

		planeGeometryInstanceId = xerScene.createGeometryInstance(&xerPlaneGeometry, &xerTexturedEffect,
			Matrix3x4::Translation(0.0f, -2.0f, 0.0f) * Matrix3x4::Scale(10.0f, 10.0f, 1.0f), &xerDefaultTexture);

		xerScene.createGeometryInstance(&xerSphereGeometry, &xerPlainEffect, Matrix3x4::Identity());

		//cubeGeometryInstanceId = xerScene.createGeometryInstance(&xerCubeGeometry, &xerPlainEffect, Matrix3x4::Identity());
		//monsterGeometryInstanceId = xerScene.createGeometryInstance(&xerMonsterSkinnedGeometry, &xerPlainSkinnedEffect, Matrix3x4::Identity());

		for (uint32 i = 0; i < 50; i++)
		{
			xerScene.createGeometryInstance(&xerSphereGeometry, &xerPlainEffect,
				Matrix3x4::Translation(Random::Global.getF32(-3.0f, 3.0f), Random::Global.getF32(-3.0f, 3.0f), Random::Global.getF32(-3.0f, 3.0f)) *
				Matrix3x4::Scale(0.5f, 0.5f, 0.5f) *
				Matrix3x4::RotationX(Random::Global.getF32(0.0f, 3.14f)) * Matrix3x4::RotationY(Random::Global.getF32(0.0f, 3.14f)));
		}

		/*for each (XERGeometryInstanceId& id in cubesCircleIds)
			id = xerScene.createGeometryInstance(&xerCubeGeometry, &xerPlainEffect, Matrix3x4::Identity());*/

		light1Id = xerScene.createLight(float32x3(3.0f, 0.0f, 0.0f), 0x0000FF_rgb, 4.0f);
		light2Id = xerScene.createLight(float32x3(0.0f, 10.0f, 2.0f), 0xFF4040_rgb, 6.0f);
		light3Id = xerScene.createLight(float32x3(8.0f, 11.0f, 10.0f), 0xFFFFFF_rgb, 30.0f);
		light4Id = xerScene.createLight(float32x3(-10.0f, -11.0f, -10.0f), 0x00FFFF_rgb, 30.0f);

		xerCamera.position = { 0.0f, 0.0f, 0.0f };
		xerCamera.fov = 1.0f;
		xerCameraRotation.angles = { 0.0f, 0.0f };

		lastFrameTimerRecord = Timer::GetRecord();
	}
	virtual void onResize(ResizingArgs& args) override
	{
		xerWindowTarget.resize(args.width, args.height);
	}
	virtual void onKeyboard(VirtualKey key, bool state) override
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

		case VirtualKey('R'):
			if (state)
			{
				frameTimeSum = 0.0f;
				frameCount = 0;
			}
			break;

		case VirtualKey('O'):
			if (state)
				ocUpdatesEnabled = !ocUpdatesEnabled;
			break;

		case VirtualKey('U'):
			if (state && !ocUpdatesEnabled)
			{
				ocUpdatesEnabled = true;
				update();
				ocUpdatesEnabled = false;
			}
			break;

		case VirtualKey('T'):
			if (state)
			{
				float32x3 lastPosition = xerCamera.position;
				XERCameraRotation lastRotation = xerCameraRotation;

				xerCamera.position = { 0.0f, -3.0f, -10.0f };
				xerCameraRotation.angles = { 1.0f, 0.0f };
				update();
				Thread::Sleep(1000);
				xerCameraRotation.angles = { 0.0f, 0.0f };
				update();
				Thread::Sleep(1000);
				update();
				Thread::Sleep(1000);

				xerCamera.position = lastPosition;
				xerCameraRotation = lastRotation;
			}
			break;
		}
	}
	virtual void onMouseButton(MouseState& mouseState, MouseButton button, bool state) override
	{
		if (button == MouseButton::Left)
		{
			captureMouseToCameraRotation = state;
			if (state)
				mouseLastPos = { mouseState.x, mouseState.y };
		}
	}
	virtual void onMouseMove(MouseState& state) override
	{
		if (captureMouseToCameraRotation)
		{
			xerCameraRotation.angles.x += float32(state.x - mouseLastPos.x) * 0.005f;
			xerCameraRotation.angles.y += float32(state.y - mouseLastPos.y) * 0.005f;
			mouseLastPos = { state.x, state.y };
		}
	}
	virtual void onCharacter(wchar character) override
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

public:
	void update()
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

		if (controls.coefUp)
			coef *= 1.01f;
		if (controls.coefDown)
			coef /= 1.01f;

		xerCameraRotation.setCameraForwardAndTranslateRotated_cameraVertical(xerCamera, translation);

		//scene.setTransform(cube1, Matrix3x4::RotationY(a += 0.01f));
		//scene.setLightPosition(light2, float32x3(0.0f, 1.5f, 1.0f) + xyz(float32x3(0.0f, 3.5f, 1.0f) * Matrix4x4::RotationX(a * 4.0f)));

		// TODO: invalid specular lighting. debug
		// camera	{position={x=-1.25275731 y=-0.933473468 z=1.04755759 ...} forward={x=0.531684399 y=0.807557702 z=0.255268931 ...} ...}

		time += coef;
		//xerScene.setGeometryInstanceTransform(monsterGeometryInstanceId, Matrix3x4::RotationY(time));

		/*static float32 angleDelta = Math::PiF32 * 2.0f / float32(countof(cubesCircleIds));
		for (uint32 i = 0; i < countof(cubesCircleIds); i++)
		{
			Matrix3x4 transform =
				Matrix3x4::RotationY(i * angleDelta + time) *
				Matrix3x4::Translation(2.0f, 0.0f, 0.0f) *
				Matrix3x4::Scale(0.3f, 0.3f, 0.3f);
			xerScene.setGeometryInstanceTransform(cubesCircleIds[i], transform);
		}*/

		//camera.position = { -1.25275731f, -0.933473468f, 1.04755759f };
		//camera.forward = { 0.531684399f, 0.807557702f, 0.255268931f };

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

			char buffer[512];
			sprintf(buffer, "XEngine v0.0001 by RBMKP4800\nRunning %s\n"
				"Total frame time %5.2f ms\n"
				"OP %5.2f ms\n"
				"OC %5.2f ms (%5.2f %5.2f %5.2f)\n"
				"LP %5.2f ms\n"
				"OC updates %s\n"
				"Cam (%4.2f %4.2f %4.2f) -> (%4.2f %4.2f %4.2f)\n"
				"WORK IN PROGRESS",
				xerDevice.getName(), totalFrameTime * 1000.0f, objectPassTime * 1000.0f,
				occlusionCullingTime * 1000.0f,
				occlusionCullingDownscaleTime * 1000.0f,occlusionCullingBBoxDrawTime * 1000.0f, occlusionCullingCommandListUpdateTime * 1000.0f,
				ligtingPassTime * 1000.0f,
				ocUpdatesEnabled ? "enabled" : "disabled",
				xerCamera.position.x, xerCamera.position.y, xerCamera.position.z,
				xerCamera.forward.x, xerCamera.forward.y, xerCamera.forward.z);

			xerUIRender.drawText(&xerFont, float32x2(10.0f, 10.0f), buffer, 0xFFFF00_rgb);
		}

		if (consoleEnabled)
			xerUIRender.drawConsole(&xerConsole);

		xerUIRender.endDraw();
		xerWindowTarget.present(true);
	}
};

void Program::Run()
{
	Window window;
	window.create(1440, 900, L"XEngine", true);

	while (window.isOpened())
	{
		WindowBase::DispatchPending();
		window.update();
	}
}