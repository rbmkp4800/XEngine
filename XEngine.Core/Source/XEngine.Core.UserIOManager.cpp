#include "XEngine.Core.UserIOManager.h"

using namespace XLib;
using namespace XEngine::Core;

uint32 __stdcall UserIOManager::DispatchThreadMain(UserIOManager* self)
{
	self->dispatchThreadMain();
	return 0;
}

void UserIOManager::dispatchThreadMain()
{
	window.create(1280, 720, L"XEngine");

	controlEvent.set();

	WindowBase::DispatchAll();
}

void UserIOManager::initialize()
{
	controlEvent.initialize(false);
	dispatchThread.create(&DispatchThreadMain, this);

	controlEvent.wait();
}

void UserIOManager::destroy()
{

}