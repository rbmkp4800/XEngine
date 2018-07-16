//#include <XLib.PoolAllocator.h>
#include <XEngine.Render.Device.h>
#include <XEngine.Render.Vertices.h>

#include "XEngine.Core.GeometryResource.h"

#include "XEngine.Core.Engine.h"

using namespace XLib;
using namespace XEngine::Core;
using namespace XEngine::Render;

/*namespace
{
	struct Request
	{
		GeometryResource *resource = nullptr;
	};

	using RequestsAllocator = PoolAllocator<Request,
		PoolAllocatorHeapUsagePolicy::SingleDynamicChunk>;

	RequestsAllocator requestsAllocator;
}*/

GeometryResource::~GeometryResource()
{

}

//bool GeometryResource::createFromFileAsync(const char* filename)
//{
//
//}

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

	static uint16 indices[] =
	{
		 0,  1,  2,
		 0,  3,  1,

		 4,  6,  5,
		 4,  5,  7,

		 8, 10,  9,
		 8,  9, 11,

		12, 14, 13,
		12, 13, 15,

		16, 18, 17,
		16, 17, 19,

		20, 22, 21,
		20, 21, 23,
	};

	vertexCount = countof(vertices);
	indexCount = countof(indices);
	vertexStride = sizeof(VertexBase);
	indexIs32Bit = false;

	buffer = Engine::GetRenderDevice().createBuffer(sizeof(vertices) + sizeof(indices));
	Engine::GetRenderDevice().updateBuffer(buffer, 0, vertices, sizeof(vertices));
	Engine::GetRenderDevice().updateBuffer(buffer, sizeof(vertices), indices, sizeof(indices));
}

void GeometryResource::cancelCreation()
{

}

bool GeometryResource::isReady() const
{
	return true;
}

void GeometryResource::setReadyCallback() const
{

}