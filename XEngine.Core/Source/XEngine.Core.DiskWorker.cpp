#include "XEngine.Core.DiskWorker.h"

using namespace XLib;
using namespace XEngine::Core;

uint32 __stdcall DiskWorker::DispatchThreadMain(DiskWorker* self)
{
	self->dispatchThreadMain();
	return 0;
}

void DiskWorker::dispatchThreadMain()
{
	for (;;)
	{

	}
}

void DiskWorker::initialize(uint32 bufferSize)
{
	dispatchThread.create(&DispatchThreadMain);
}