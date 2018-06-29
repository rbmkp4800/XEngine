#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.System.Threading.h>
#include <XLib.System.Threading.Lock.h>
#include <XLib.Delegate.h>
#include <XLib.PoolAllocator.h>
#include <XLib.Containers.IntrusiveDoublyLinkedList.h>

namespace XEngine::Core
{
	using DiskOperationHandle = uint32;

	using DiskReadHandler = XLib::Delegate<void, bool, const void*, uint32, void*>;

	class DiskWorker : public XLib::NonCopyable
	{
	private:
		struct OperationEntry
		{
			XLib::IntrusiveDoublyLinkedListNodeHook entriesListNodeHook;

			DiskReadHandler handler;
		};

		using OperationEntriesAllocator =
			XLib::PoolAllocator<OperationEntry, XLib::PoolAllocatorHeapUsagePolicy::MultipleStaticChunks<5, 12>>;

		using OperationEntriesList =
			XLib::IntrusiveDoublyLinkedList<OperationEntry, &OperationEntry::entriesListNodeHook>;

	private:
		OperationEntriesAllocator operationEntriesAllocator;
		OperationEntriesList operationEntriesList;

		XLib::Thread dispatchThread;
		XLib::Lock lock;

	private:
		static uint32 __stdcall DispatchThreadMain(DiskWorker* self);
		void dispatchThreadMain();

	public:
		DiskWorker() = default;
		~DiskWorker() = default;

		void initialize(uint32 bufferSize);

		DiskOperationHandle readFile(const char* filename, DiskReadHandler handler, void* context);
		
		void cancelOperation(DiskOperationHandle handle);
	};
}