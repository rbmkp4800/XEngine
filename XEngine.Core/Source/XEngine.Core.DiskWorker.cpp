#include <XLib.System.File.h>

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
		operationsWaitEvent.wait();

		Operation *operation = nullptr;
		{
			ScopedLock lock(operationsQueueLock);
			operation = operationsQueue.popFront();
		}

		File file;
		if (!file.open(..., FileAccessMode::Read, FileOpenMode::OpenExisting))
			operation->handler.call(false, nullptr, 0, operation->handlerContext);

		file.read();

		operation->handler.call(true, , 0, operation->handlerContext);

		operationsAllocator.release(operation);
	}
}

void DiskWorker::startup(uint32 bufferSize)
{
	// TODO: check if already running

	this->bufferSize = bufferSize;
	buffer.resize(bufferSize);

	operationsWaitEvent.initialize(false);
	dispatchThread.create(&DispatchThreadMain);
}

void DiskWorker::shutdown()
{

}

DiskOperationHandle DiskWorker::readFile(const char* filename,
	DiskReadHandler handler, void* context)
{

}