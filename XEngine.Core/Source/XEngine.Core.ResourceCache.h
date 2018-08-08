#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.PoolAllocator.h>
#include <XLib.Containers.IntrusiveSet.h>
#include <XLib.Delegate.h>

// TODO: replace set with hash-set
// TODO: handle allocations properly

namespace XEngine::Core
{
	template <typename ResourceUID,
		typename ResourceHandle,
		typename Resource>
	class ResourceCache : public XLib::NonCopyable
	{
	private:
		struct ResourceCreationTask : public Resource::CreationTask
		{

		};

		struct ResourceEntry
		{
			XLib::IntrusiveSetNodeHook uidMappingSetHook;
			ResourceUID uid;
			union
			{
				Resource resource;
				ResourceCreationTask *resourceCreationTask = nullptr;
			};

			uint16 referenceCount = 0;
			uint16 handleGeneration = 0;
		};

		using Allocator = XLib::PoolAllocator<ResourceEntry,
			XLib::PoolAllocatorHeapUsagePolicy::MultipleStaticChunks<4, 14>>;

		using UIDMappingSet = XLib::IntrusiveSet<ResourceEntry, &ResourceEntry::uidMappingSetHook>;

	private:
		Allocator allocator;
		UIDMappingSet uidMappingSet;

	public:
		ResourceCache() = default;
		~ResourceCache() = default;

		ResourceHandle create(ResourceUID uid);

		const ResourceHandle find(ResourceUID uid, bool addReference = true);
		const Resource& get(ResourceHandle handle);

		void addReference(ResourceHandle handle);
		void removeReference(ResourceHandle handle);
		bool isValidHandle(ResourceHandle handle);

		bool isReady(ResourceHandle handle);
		bool setReadyCallback(ResourceHandle handle, XLib::Delegate<void> callback);
	};
}