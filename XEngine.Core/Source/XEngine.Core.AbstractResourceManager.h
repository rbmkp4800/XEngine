#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.PoolAllocator.h>
#include <XLib.Containers.IntrusiveSet.h>

namespace XEngine::Core
{
	template <typename ResourceUID,
		typename ResourceHandle,
		typename Resource>
	class AbstractResourceManager : public XLib::NonCopyable
	{
	private:
		struct ResourceEntry
		{
			XLib::IntrusiveSetNodeHook uidMappingSetHook;
			ResourceUID uid;
			Resource desc;

			uint16 referenceCount;
			uint16 handleGeneration;
		};

		using Allocator = XLib::PoolAllocator<ResourceEntry,
			XLib::PoolAllocatorHeapUsagePolicy::MultipleStaticChunks<4, 14>>;

		using UIDMappingSet = XLib::IntrusiveSet<ResourceEntry, &ResourceEntry::uidMappingSetHook>;

	private:
		Allocator allocator;
		UIDMappingSet uidMappingSet;

	public:
		AbstractResourceManager() = default;
		~AbstractResourceManager() = default;

		ResourceHandle create(ResourceUID uid);

		const Resource& find(ResourceUID uid);
		ResourceHandle findHandle(ResourceUID uid);
		const Resource& get(ResourceHandle handle);

		void addReference(ResourceHandle handle);
		void removeReference(ResourceHandle handle);

		bool isValidHandle(ResourceHandle handle);
	};
}