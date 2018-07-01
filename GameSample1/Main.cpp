#include <XLib.Program.h>
#include <XEngine.Core.Engine.h>
#include <XEngine.Core.Input.h>
#include <XEngine.Render.Scene.h>

using namespace XLib;
using namespace XEngine;

class GameSample :
	public Core::GameBase,
	public Core::InputHandler
{
private:
	Render::Scene scene;

protected:
	virtual void initialize() override
	{
		Core::Input::AddHandler(this);
	}

	virtual void update(float32 timeDelta) override
	{

	}

	virtual void onMouseMove(sint16x2 delta) override {}

	virtual void onMouseButton(XLib::MouseButton button, bool state) override {}

	virtual void onMouseWheel(float32 delta) override {}

	virtual void onKeyboard(VirtualKey key, bool state) override
	{
		if (key == VirtualKey::Escape && state)
			Core::Engine::Shutdown();
	}

	virtual void onCloseRequest() override
	{
		Core::Engine::Shutdown();
	}
};

void Program::Run()
{
	GameSample game;
	Core::Engine::Run(&game);
}