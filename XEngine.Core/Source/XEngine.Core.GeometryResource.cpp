#include <XLib.Vectors.Math.h>
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

	Render::Device& renderDevice = Engine::GetRenderDevice();
	buffer = renderDevice.createBuffer(sizeof(vertices) + sizeof(indices));
	renderDevice.updateBuffer(buffer, 0, vertices, sizeof(vertices));
	renderDevice.updateBuffer(buffer, sizeof(vertices), indices, sizeof(indices));
}

void GeometryResource::createCubicSphere(uint32 detalizationLevel)
{
	// TODO: check max detalization value

	uint16 quadsPerEdge = uint16(detalizationLevel) + 1;
	uint16 vertexPerEdge = quadsPerEdge + 1;
	uint16 vertexPerFace = sqr(vertexPerEdge);
	uint16 indexPerFace = sqr(quadsPerEdge) * 6;

	HeapPtr<byte> temp((sizeof(VertexBase) * vertexPerFace + sizeof(uint16) * indexPerFace) * 6);
	VertexBase *vertexBuffer = to<VertexBase*>(temp);
	uint16 *indexBuffer = to<uint16*>(vertexBuffer + vertexPerFace * 6);

	// Vertices generation
	{
		VertexBase *faceVertices[6];
		for (uint16 i = 0; i < 6; i++)
			faceVertices[i] = vertexBuffer + i * vertexPerFace;

		for (uint16 row = 0; row < vertexPerEdge; row++)
		{
			for (uint16 column = 0; column < vertexPerEdge; column++)
			{
				float32x3 flatPosition =
				{
					1.0f,
					lerp(-1.0f, 1.0f, column, quadsPerEdge),
					lerp(-1.0f, 1.0f, row, quadsPerEdge)
				};

				float32x3 p = VectorMath::Normalize(flatPosition);
				uint16 offset = row * vertexPerEdge + column;
				faceVertices[0][offset].normal = faceVertices[0][offset].position = { -p.x,  p.y,  p.z };
				faceVertices[1][offset].normal = faceVertices[1][offset].position = {  p.x, -p.y,  p.z };
				faceVertices[2][offset].normal = faceVertices[2][offset].position = {  p.y,  p.x,  p.z };
				faceVertices[3][offset].normal = faceVertices[3][offset].position = { -p.y, -p.x,  p.z };
				faceVertices[4][offset].normal = faceVertices[4][offset].position = {  p.z,  p.y,  p.x };
				faceVertices[5][offset].normal = faceVertices[5][offset].position = {  p.z, -p.y, -p.x };
			}
		}
	}

	// Indices generation
	for (uint16 face = 0; face < 6; face++)
	{
		uint16 *faceIndices = indexBuffer + face * indexPerFace;

		for (uint16 row = 0; row < quadsPerEdge; row++)
		{
			for (uint16 column = 0; column < quadsPerEdge; column++)
			{
				uint16 leftTop = vertexPerFace * face + row * vertexPerEdge + column;
				uint16 leftBottom = leftTop + vertexPerEdge;
				uint16 rightTop = leftTop + 1;
				uint16 rightBottom = leftBottom + 1;

				uint16 *quadIndices = &faceIndices[(row * quadsPerEdge + column) * 6];
				quadIndices[0] = leftTop;
				quadIndices[1] = leftBottom;
				quadIndices[2] = rightTop;
				quadIndices[3] = rightBottom;
				quadIndices[4] = rightTop;
				quadIndices[5] = leftBottom;
			}
		}
	}

	vertexCount = vertexPerFace * 6;
	indexCount = indexPerFace * 6;
	vertexStride = sizeof(VertexBase);
	indexIs32Bit = false;

	uint32 verticesSize = vertexCount * sizeof(VertexBase);
	uint32 indicesSize = indexCount * sizeof(uint16);

	Render::Device& renderDevice = Engine::GetRenderDevice();
	buffer = renderDevice.createBuffer(verticesSize + indicesSize);
	renderDevice.updateBuffer(buffer, 0, vertexBuffer, verticesSize);
	renderDevice.updateBuffer(buffer, verticesSize, indexBuffer, indicesSize);
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