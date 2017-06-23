#include <XLib.Util.h>
#include <XLib.Heap.h>
#include <XLib.Float.Conversion.h>
#include <XLib.Vectors.Math.h>

#include "XEngine.Render.GeometryGenerator.h"
#include "XEngine.Render.GeometryGenerator.SampleGeometries.h"
#include "XEngine.Render.Resources.h"
#include "XEngine.Render.Vertices.h"

using namespace XLib;
using namespace XEngine;

void XERGeometryGenerator::HorizontalPlane(XERDevice* device, XERGeometry* geometry)
{
	static VertexTexture vertices[] =
	{
		{ { -1.0f, -1.0f, 0.0f },	{ 0.0f, 0.0f, 1.0f },	{ 0.0f, 0.0f } },
		{ { 1.0f, 1.0f, 0.0f },		{ 0.0f, 0.0f, 1.0f },	{ 1.0f, 1.0f } },
		{ { -1.0f, 1.0f, 0.0f },	{ 0.0f, 0.0f, 1.0f },	{ 0.0f, 1.0f } },
		{ { 1.0f, -1.0f, 0.0f },	{ 0.0f, 0.0f, 1.0f },	{ 1.0f, 0.0f } },
	};

	static uint32 indices[] =
	{
		0, 1, 2,
		0, 3, 1,
	};

	geometry->initialize(device, vertices, countof(vertices), sizeof(vertices[0]), indices, countof(indices));
}

void XERGeometryGenerator::Cube(XERDevice* device, XERGeometry* geometry)
{
	static VertexBase vertices[] =
	{
		{ { -1.0f, -1.0f, -1.0f },{ 0.0f, 0.0f, -1.0f } },
		{ { 1.0f, 1.0f, -1.0f },{ 0.0f, 0.0f, -1.0f } },
		{ { 1.0f, -1.0f, -1.0f },{ 0.0f, 0.0f, -1.0f } },
		{ { -1.0f, 1.0f, -1.0f },{ 0.0f, 0.0f, -1.0f } },

		{ { -1.0f, -1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { 1.0f, -1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },

		{ { -1.0f, 1.0f, -1.0f, },{ 0.0f, 1.0f, 0.0f } },
		{ { 1.0f, 1.0f, 1.0f, },{ 0.0f, 1.0f, 0.0f } },
		{ { -1.0f, 1.0f, 1.0f, },{ 0.0f, 1.0f, 0.0f } },
		{ { 1.0f, 1.0f, -1.0f, },{ 0.0f, 1.0f, 0.0f } },

		{ { -1.0f, -1.0f, -1.0f, },{ 0.0f, -1.0f, 0.0f } },
		{ { 1.0f, -1.0f, 1.0f, },{ 0.0f, -1.0f, 0.0f } },
		{ { 1.0f, -1.0f, -1.0f, },{ 0.0f, -1.0f, 0.0f } },
		{ { -1.0f, -1.0f, 1.0f, },{ 0.0f, -1.0f, 0.0f } },

		{ { -1.0f, -1.0f, -1.0f },{ -1.0f, 0.0f, 0.0f } },
		{ { -1.0f, 1.0f, 1.0f },{ -1.0f, 0.0f, 0.0f } },
		{ { -1.0f, -1.0f, 1.0f },{ -1.0f, 0.0f, 0.0f } },
		{ { -1.0f, 1.0f, -1.0f },{ -1.0f, 0.0f, 0.0f } },

		{ { 1.0f, -1.0f, -1.0f },{ 1.0f, 0.0f, 0.0f } },
		{ { 1.0f, 1.0f, 1.0f },{ 1.0f, 0.0f, 0.0f } },
		{ { 1.0f, 1.0f, -1.0f },{ 1.0f, 0.0f, 0.0f } },
		{ { 1.0f, -1.0f, 1.0f },{ 1.0f, 0.0f, 0.0f } },
	};

	static uint32 indices[] =
	{
		0, 1, 2,
		0, 3, 1,

		4, 6, 5,
		4, 5, 7,

		8, 10, 9,
		8, 9, 11,

		12, 14, 13,
		12, 13, 15,

		16, 18, 17,
		16, 17, 19,

		20, 22, 21,
		20, 21, 23,
	};

	geometry->initialize(device, vertices, countof(vertices), sizeof(VertexBase), indices, countof(indices));
}

void XERGeometryGenerator::Sphere(uint32 detalizationLevel, XERDevice* device, XERGeometry* geometry)
{
	uint32 quadsPerEdge = detalizationLevel + 1;
	uint32 vertexPerEdge = quadsPerEdge + 1;
	uint32 vertexPerFace = sqr(vertexPerEdge);
	uint32 indexPerFace = sqr(quadsPerEdge) * 6;

	HeapPtr<byte> buffer((sizeof(VertexBase) * vertexPerFace + sizeof(uint32) * indexPerFace) * 6);
	VertexBase *vertexBuffer = to<VertexBase*>(buffer);
	uint32 *indexBuffer = to<uint32*>(vertexBuffer + vertexPerFace * 6);

	{
		VertexBase *faceVertices[6];
		for (uint32 i = 0; i < 6; i++)
			faceVertices[i] = vertexBuffer + i * vertexPerFace;

		for (uint32 row = 0; row < vertexPerEdge; row++)
		{
			for (uint32 column = 0; column < vertexPerEdge; column++)
			{
				float32x3 flatPosition =
				{
					1.0f,
					lerp(-1.0f, 1.0f, column, quadsPerEdge),
					lerp(-1.0f, 1.0f, row, quadsPerEdge)
				};

				float32x3 p = VectorMath::Normalize(flatPosition);
				uint32 offset = row * vertexPerEdge + column;
				faceVertices[0][offset].normal = faceVertices[0][offset].position = { -p.x, p.y, p.z };
				faceVertices[1][offset].normal = faceVertices[1][offset].position = { p.x, -p.y, p.z };
				faceVertices[2][offset].normal = faceVertices[2][offset].position = { p.y, p.x, p.z };
				faceVertices[3][offset].normal = faceVertices[3][offset].position = { -p.y, -p.x, p.z };
				faceVertices[4][offset].normal = faceVertices[4][offset].position = { p.z, p.y, p.x };
				faceVertices[5][offset].normal = faceVertices[5][offset].position = { p.z, -p.y, -p.x };
			}
		}
	}

	{
		uint32 *faceIndices[6];
		for (uint32 i = 0; i < 6; i++)
			faceIndices[i] = indexBuffer + i * indexPerFace;

		for (uint32 face = 0; face < 6; face++)
		{
			for (uint32 row = 0; row < quadsPerEdge; row++)
			{
				for (uint32 column = 0; column < quadsPerEdge; column++)
				{
					uint32 leftTop = vertexPerFace * face + row * vertexPerEdge + column;
					uint32 leftBottom = leftTop + vertexPerEdge;
					uint32 rightTop = leftTop + 1;
					uint32 rightBottom = leftBottom + 1;

					uint32 *quadIndices = &faceIndices[face][(row * quadsPerEdge + column) * 6];
					quadIndices[0] = leftTop;
					quadIndices[1] = leftBottom;
					quadIndices[2] = rightTop;
					quadIndices[3] = rightBottom;
					quadIndices[4] = rightTop;
					quadIndices[5] = leftBottom;
				}
			}
		}
	}

	geometry->initialize(device, vertexBuffer, vertexPerFace * 6,
		sizeof(VertexBase), indexBuffer, indexPerFace * 6);
}

void XERGeometryGenerator::MonsterSkinned(XERDevice* device, XERGeometry* geometry)
{
	geometry->initialize(device, monsterSkinnedVertices, countof(monsterSkinnedVertices),
		sizeof(monsterSkinnedVertices[0]), monsterIndices, countof(monsterIndices));
}