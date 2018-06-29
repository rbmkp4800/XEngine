#include <XLib.PoolAllocator.h>

#include "XEngine.Core.GeometryResource.h"

using namespace XLib;
using namespace XEngine::Core;
using namespace XEngine::Render;

namespace
{
	struct Request
	{
		GeometryResource *resource;
	};

	using RequestsAllocator = PoolAllocator<Request,
		PoolAllocatorHeapUsagePolicy::SingleDynamicChunk>;

	RequestsAllocator requestsAllocator;
}

GeometryResource::~GeometryResource()
{

}

bool GeometryResource::createFromFileAsync(const char* filename)
{

}

void GeometryResource::createCube()
{

}

void GeometryResource::cancelCreation()
{

}

bool GeometryResource::isReady() const
{

}

void GeometryResource::setReadyCallback() const
{

}