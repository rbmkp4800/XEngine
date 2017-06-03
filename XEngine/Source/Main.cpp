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
	XERContext xerContext;
	XERWindowTarget xerWindowTarget;

	XERUIGeometryRenderer xerUIRenderer;
	XERMonospacedFont xerFont;

	XERScene xerScene;
	XEREffect xerPlainEffect;
	XEREffect xerPlainSkinnedEffect;
	XERGeometry xerPlaneGeometry;
	XERGeometry xerCubeGeometry;
	//XERGeometry xerMonsterGeometry;
	XERGeometry xerMonsterSkinnedGeometry;

	XERGeometryInstanceId cubesCircleIds[10];

	XERGeometryInstanceId planeGeometryInstanceId;
	XERGeometryInstanceId cubeGeometryInstanceId;
	XERGeometryInstanceId monsterGeometryInstanceId;
	XERLightId light1Id, light2Id, light3Id, light4Id;

	//float32x3 cameraPosition = { 0.0f, 0.0f, -10.0f };
	//float32x2 cameraRotation = { 0.0f, 0.0f };
	XERCamera camera;
	float32x2 cameraRotation = { 0.0f, 0.0f };
	sint16x2 mouseLastPos = { 0, 0 };
	bool captureMouseToCameraRotation = false;

	struct { bool left, right, up, down, forward, backward, wireframe, narrowFOV, coefUp, coefDown; } controls = {};

	float32 time = 0.0f;
	float32 coef = 0.01f;

	float frameTimeSum = 0.0f;
	uint32 frameCount = 0;
	TimerRecord lastFrameTimerRecord;

	virtual void onCreate(CreationArgs& args) override
	{
		xerDevice.initialize();
		xerContext.initialize(&xerDevice);
		xerWindowTarget.initialize(&xerDevice, this->getHandle(), args.width, args.height);

		xerUIRenderer.initialize(&xerDevice);
		XERResourceLoader::LoadDefaultFont(&xerDevice, &xerFont);
		
		xerScene.initialize(&xerDevice);
		xerPlainEffect.initializePlain(&xerDevice);
		xerPlainSkinnedEffect.initializePlainSkinned(&xerDevice);

		XERGeometryGenerator::Plane(&xerDevice, &xerPlaneGeometry);
		XERGeometryGenerator::Cube(&xerDevice, &xerCubeGeometry);
		//XERGeometryGenerator::Monster(&xerDevice, &xerMonsterGeometry);
		XERGeometryGenerator::MonsterSkinned(&xerDevice, &xerMonsterSkinnedGeometry);
		planeGeometryInstanceId = xerScene.createGeometryInstance(&xerPlaneGeometry, &xerPlainEffect, Matrix3x4::Translation(0.0f, -2.0f, 0.0f) * Matrix3x4::Scale(10.0f, 10.0f, 10.0f));
		//cubeGeometryInstanceId = xerScene.createGeometryInstance(&xerCubeGeometry, &xerPlainEffect, Matrix3x4::Identity());
		//monsterGeometryInstanceId = xerScene.createGeometryInstance(&xerMonsterSkinnedGeometry, &xerPlainSkinnedEffect, Matrix3x4::Identity());

		for (uint32 i = 0; i < 60000; i++)
		{
			xerScene.createGeometryInstance(&xerCubeGeometry, &xerPlainEffect,
				Matrix3x4::Translation(Random::Global.getF32(-3.0f, 3.0f), Random::Global.getF32(-3.0f, 3.0f), Random::Global.getF32(-3.0f, 3.0f)) *
				Matrix3x4::Scale(0.05f, 0.05f, 0.05f) *
				Matrix3x4::RotationX(Random::Global.getF32(0.0f, 3.14f)) * Matrix3x4::RotationY(Random::Global.getF32(0.0f, 3.14f)));
		}

		for each (XERGeometryInstanceId& id in cubesCircleIds)
			id = xerScene.createGeometryInstance(&xerCubeGeometry, &xerPlainEffect, Matrix3x4::Identity());

		light1Id = xerScene.createLight(float32x3(3.0f, 0.0f, 0.0f), 0x0000FF_rgb, 4.0f);
		light2Id = xerScene.createLight(float32x3(0.0f, 10.0f, 2.0f), 0xFF4040_rgb, 6.0f);
		light3Id = xerScene.createLight(float32x3(8.0f, 11.0f, 10.0f), 0xFFFFFF_rgb, 30.0f);
		light4Id = xerScene.createLight(float32x3(5.0f, -11.0f, -10.0f), 0x00FFFF_rgb, 30.0f);

		camera.position = { 0.0f, -1.0f, -5.0f };

		lastFrameTimerRecord = Timer::GetRecord();
	}
	virtual void onKeyboard(VirtualKey key, bool state) override
	{
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
				camera.fov = 0.1f;
			else
				camera.fov = 1.0f;
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
				break;
			}
		}
	}
	virtual void onMouseButton(MouseState& mouseState, MouseButton button, bool state) override
	{
		if (button == MouseButton::Right)
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
			cameraRotation.x += float32(state.x - mouseLastPos.x) * 0.005f;
			cameraRotation.y += float32(state.y - mouseLastPos.y) * 0.005f;
			mouseLastPos = { state.x, state.y };
		}
	}

public:
	void update()
	{
		camera.forward = VectorMath::SphericalCoords(cameraRotation);

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

		camera.translateInViewSpace(translation);

		//scene.setTransform(cube1, Matrix3x4::RotationY(a += 0.01f));
		//scene.setLightPosition(light2, float32x3(0.0f, 1.5f, 1.0f) + xyz(float32x3(0.0f, 3.5f, 1.0f) * Matrix4x4::RotationX(a * 4.0f)));

		// TODO: invalid specular lighting. debug
		// camera	{position={x=-1.25275731 y=-0.933473468 z=1.04755759 ...} forward={x=0.531684399 y=0.807557702 z=0.255268931 ...} ...}

		time += coef;
		//xerScene.setGeometryInstanceTransform(monsterGeometryInstanceId, Matrix3x4::RotationY(time));

		static float32 angleDelta = Math::PiF32 * 2.0f / float32(countof(cubesCircleIds));
		for (uint32 i = 0; i < countof(cubesCircleIds); i++)
		{
			Matrix3x4 transform =
				Matrix3x4::RotationY(i * angleDelta + time) *
				Matrix3x4::Translation(2.0f, 0.0f, 0.0f) *
				Matrix3x4::Scale(0.3f, 0.3f, 0.3f);
			xerScene.setGeometryInstanceTransform(cubesCircleIds[i], transform);
		}

		//camera.position = { -1.25275731f, -0.933473468f, 1.04755759f };
		//camera.forward = { 0.531684399f, 0.807557702f, 0.255268931f };

		xerUIRenderer.beginDraw(&xerContext);
		xerUIRenderer.setFont(&xerFont);
		{
			float32 frameTime = Timer::GetTimeDelta_UpdateRecord(lastFrameTimerRecord) * 1000.0f;
			frameTimeSum += frameTime;
			frameCount++;

			char buffer[256];
			sprintf(buffer, "XEngine v0.0001 by RBMKP4800\nRunning %s\nFrame time %4.1f ms (avg %5.2f)\nWORK IN PROGRESS",
				xerDevice.getName(), frameTime, frameTimeSum / float32(frameCount));
			xerUIRenderer.drawText(float32x2(10.0f, 10.0f), buffer);
		}
		xerUIRenderer.endDraw();

		XERTargetBuffer *xerTarget = xerWindowTarget.getCurrentTargetBuffer();
		xerContext.draw(xerTarget, &xerScene, camera, controls.wireframe ? XERDebugWireframeMode::Enabled : XERDebugWireframeMode::Disabled);
		xerContext.draw(xerTarget, &xerUIRenderer);
		xerWindowTarget.present(false);
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