#include <XLib.PoolAllocator.h>
#include <XEngine.Render.Device.h>
#include <XEngine.Render.Vertices.h>

#include "XEngine.Core.GeometryResource.h"

#include "XEngine.Core.Engine.h"

using namespace XLib;
using namespace XEngine::Core;
using namespace XEngine::Render;

namespace
{
	struct Request
	{
		GeometryResource *resource = nullptr;
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
	static VertexBase vertices[] =
	{
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f } },

		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f } },

		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },

		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, -1.0f, 0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 0.0f, -1.0f, 0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 0.0f, -1.0f, 0.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, -1.0f, 0.0f } },

		{ { -1.0f, -1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { -1.0f, 0.0f, 0.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { -1.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f } },

		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 0.0f } },
	};

	BufferHandle buffer = Engine::GetRenderDevice().createBuffer(sizeof(vertices));
	//Engine::GetRenderDevice().uploadBuffer();
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