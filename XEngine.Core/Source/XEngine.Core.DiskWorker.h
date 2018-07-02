#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Delegate.h>
#include <XLib.PoolAllocator.h>
#include <XLib.Containers.IntrusiveDoublyLinkedList.h>
#include <XLib.System.Threading.h>
#include <XLib.System.Threading.Lock.h>
#include <XLib.System.Threading.Event.h>
#include <XLib.Heap.h>

namespace XEngine::Core
{
	using DiskOperationHandle = uint32;
	using DiskReadHandler = XLib::Delegate<void, bool, const void*, uint32, void*>;

	class DiskWorker : public XLib::NonCopyable
	{
	private:
		struct Operation
		{
			XLib::IntrusiveDoublyLinkedListItemHook operationsQueueItemHook;
			DiskReadHandler handler;
			void *handlerContext = nullptr;
		};

		using OperationsAllocator = XLib::PoolAllocator<Operation,
			XLib::PoolAllocatorHeapUsagePolicy::MultipleStaticChunks<5, 12>>;

		using OperationsQueue = XLib::IntrusiveDoublyLinkedList<
			Operation, &Operation::operationsQueueItemHook>;

	private:
		OperationsAllocator operationsAllocator;
		OperationsQueue operationsQueue;
		XLib::Lock operationsQueueLock;

		XLib::Thread dispatchThread;
		XLib::Event operationsWaitEvent;

		XLib::HeapPtr<byte> buffer;
		uint32 bufferSize;

	private:
		static uint32 __stdcall DispatchThreadMain(DiskWorker* self);
		void dispatchThreadMain();

	public:
		DiskWorker() = default;
		~DiskWorker();

		void startup(uint32 bufferSize);
		void shutdown();

		DiskOperationHandle readFile(const char* filename,
			DiskReadHandler handler, void* context);
		
		void cancelOperation(DiskOperationHandle handle);
	};
}