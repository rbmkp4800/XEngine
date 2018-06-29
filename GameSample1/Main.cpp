#include <XLib.Program.h>
#include <XEngine.Core.Engine.h>
#include <XEngine.Render.Scene.h>

using namespace XLib;
using namespace XEngine;

class GameSample : public Core::GameBase
{
private:
	Render::Scene scene;

protected:
	virtual void onInitialize() override
	{

	}

	virtual void onUpdate(float32 timeDelta) override
	{

	}
};

void Program::Run()
{
	GameSample game;
	Core::Engine::Run(&game);
}