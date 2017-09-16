#include <XLib.Program.h>

#include "Sample.h"

using namespace XLib;

void Program::Run()
{
	SampleWindow window;
	window.create(1440, 900, L"XEngine");

	while (window.isOpened())
	{
		WindowBase::DispatchPending();
		window.update();
	}
}