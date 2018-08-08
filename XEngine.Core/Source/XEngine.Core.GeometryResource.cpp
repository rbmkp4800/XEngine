#include <XLib.Vectors.Math.h>
#include <XLib.System.File.h>
#include <XEngine.Render.Device.h>
#include <XEngine.Render.Vertices.h>

#include "XEngine.Core.GeometryResource.h"

#include "XEngine.Core.Engine.h"

using namespace XLib;
using namespace XEngine::Core;
using namespace XEngine::Render;

/*GeometryResource::~GeometryResource()
{

}

bool GeometryResource::Create(const CreationArgs& args, CreationTask& task)
{
	switch (args.type)
	{
		case CreationType::Cube:

			break;

		case CreationType::CubicSphere:

			break;

		case CreationType::FromFile:

			break;

		default:
			// TODO: log warning
			return false;
	}
}

bool GeometryResource::CreationTask::cancel()
{

}*/

#pragma pack(push, 1)
class XEGeometryFile abstract final
{
public:
	static constexpr uint32 Magic = 0xAABBCCDD; // TODO: replace
	static constexpr uint16 SupportedVersion = 1;

	struct Header
	{
		uint32 magic;
		uint16 version;
		uint16 vertexStride;
		uint32 vertexCount;
		uint32 indexCount;
	};
};
#pragma pack(pop)

bool GeometryResource::createFromFile(const char* filename)
{
	File file;
	if (!file.open(filename, FileAccessMode::Read))
		return false;

	XEGeometryFile::Header header;
	if (!file.read(header))
		return false;

	if (header.magic != XEGeometryFile::Magic ||
		header.version != XEGeometryFile::SupportedVersion)
	{
		return false;
	}

	uint64 expectedFileSize = uint64(header.vertexStride) * uint64(header.vertexCount) +
		uint64(header.indexCount) * sizeof(uint32) + sizeof(header);
	if (expectedFileSize != file.getSize())
		return false;

	uint32 verticesSize = header.vertexCount * header.vertexStride;
	uint32 indicesSize = header.indexCount * 4;

	HeapPtr<byte> vertices(verticesSize);
	if (!file.read(vertices, verticesSize))
		return false;

	HeapPtr<uint32> indices(header.indexCount);
	if (!file.read(indices, indicesSize ))
		return false;

	vertexCount = header.vertexCount;
	indexCount = header.indexCount;
	vertexStride = uint8(header.vertexStride);
	indexIs32Bit = true;

	Render::Device& renderDevice = Engine::GetRenderDevice();
	buffer = renderDevice.createBuffer(verticesSize + indicesSize);
	renderDevice.updateBuffer(buffer, 0, vertices, verticesSize);
	renderDevice.updateBuffer(buffer, verticesSize, indices, indicesSize);

	return true;
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