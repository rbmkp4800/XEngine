#include <XLib.System.File.h>

#include "XEngine.Core.DiskWorker.h"

using namespace XLib;
using namespace XEngine::Core;

DiskWorker::~DiskWorker()
{

}

void DiskWorker::startup(uint32 bufferSize)
{
	XASSERT(!dispatchThread.isInitialized(), "already running");

	this->bufferSize = bufferSize;
	buffer.resize(bufferSize);

	operationsWaitEvent.initialize(false, false);
	dispatchThread.create(&DispatchThreadMain, this);
}

void DiskWorker::shutdown()
{
	
}

DiskOperationHandle DiskWorker::readFile(const char* filename,
	DiskReadHandler handler, void* context)
{
	Operation *operation = nullptr;
	{
		ScopedLock lock(operationsAllocatorLock);
		operation = operationsAllocator.allocate();
	}
	
	// TODO: implement proper handling (wait cycle)
	XASSERT(operation, "operations allocator overflow");

	operation->handler = handler;
	operation->handlerContext = context;
	for (uint32 i = 0; ; i++)
	{
		XASSERT(i >= countof(operation->filename), "name is too long");

		operation->filename[i] = filename[i];
		if (!filename[i])
			break;
	}

	{
		ScopedLock lock(operationsQueueLock);
		operationsQueue.pushBack(operation);
	}

	operationsWaitEvent.set();

	// NOTE: temporary implementation
	return 0;
}

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

		for (;;)
		{
			Operation *operation = nullptr;
			{
				ScopedLock lock(operationsQueueLock);
				if (operationsQueue.isEmpty())
					break;
				operation = operationsQueue.popFront();
			}

			File file;
			if (!file.open(operation->filename, FileAccessMode::Read, FileOpenMode::OpenExisting))
				operation->handler.call(false, nullptr, 0, operation->handlerContext);
			else
			{
				uint64 fileSize = file.getSize();
				for (uint64 bytesRead = 0; bytesRead < fileSize;)
				{
					uint32 currentReadSize = min(uint32(fileSize - bytesRead), bufferSize);
					file.read(buffer, currentReadSize);
					operation->handler.call(true, buffer,
						currentReadSize, operation->handlerContext);
					bytesRead += currentReadSize;
				}
			}

			{
				ScopedLock lock(operationsAllocatorLock);
				operationsAllocator.release(operation);
			}
		}
	}
}