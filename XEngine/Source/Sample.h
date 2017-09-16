#pragma once

#include <XLib.Types.h>
#include <XLib.System.Window.h>

#include "XEngine.h"

class SampleWindow : public XLib::WindowBase
{
private:
	struct GeometryBaseEntry
	{
		XEngine::XERGeometry xerGeometry;
		uint64 id;
	};

	XEngine::XERDevice xerDevice;
	XEngine::XERWindowTarget xerWindowTarget;

	XEngine::XERSceneRender xerSceneRender;
	XEngine::XERUIRender xerUIRender;

	XEngine::XEREffect xerPlainEffect, xerTexturedEffect, xerPlainSkinnedEffect;
	XEngine::XERMonospacedFont xerFont;

	XEngine::XERScene xerScene;
	XEngine::XEUIConsole xerConsole;

	XEngine::XERCamera xerCamera;
	XEngine::XERCameraRotation xerCameraRotation;

	GeometryBaseEntry geometryBase[8];
	uint32 geometryBaseEntryCount = 0;

	uint16 width, height;
	struct { bool left, right, up, down, forward, backward, wireframe, narrowFOV, coefUp, coefDown; } controls = {};
	sint16x2 mouseLastPos = { 0, 0 };
	bool captureMouseToCameraRotation = false;
	bool ocUpdatesEnabled = false;
	bool consoleEnabled = false;

	virtual void onCreate(XLib::CreationArgs& args) override;
	virtual void onResize(XLib::ResizingArgs& args) override;
	virtual void onKeyboard(XLib::VirtualKey key, bool state) override;
	virtual void onMouseButton(XLib::MouseState& mouseState, XLib::MouseButton button, bool state) override;
	virtual void onMouseMove(XLib::MouseState& state) override;
	virtual void onCharacter(wchar character) override;

	void onConsoleCommand(const char* commandString);

public:
	void update();
};