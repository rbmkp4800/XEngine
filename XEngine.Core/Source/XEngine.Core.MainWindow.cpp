#include "XEngine.Core.MainWindow.h"

using namespace XLib;
using namespace XEngine::Core;

uint32 __stdcall MainWindow::DispatchThreadMain(MainWindow* self)
{
	self->dispatchThreadMain();
	return 0;
}

void MainWindow::dispatchThreadMain()
{
	controlEvent.set();

	window.create(1280, 720, L"XEngine");
	WindowBase::DispatchAll();
}

void MainWindow::initialize()
{
	controlEvent.initialize(false);
	dispatchThread.create(&MainWindow::DispatchThreadMain, this);

	controlEvent.wait();
}

void MainWindow::destroy()
{

}