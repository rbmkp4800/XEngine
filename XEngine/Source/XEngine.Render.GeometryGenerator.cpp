#include <XLib.Util.h>
#include <XLib.Float.Conversion.h>

#include "XEngine.Render.GeometryGenerator.h"
#include "XEngine.Render.GeometryGenerator.SampleGeometries.h"
#include "XEngine.Render.Geometry.h"
#include "XEngine.Render.Vertices.h"

using namespace XEngine;

void XERGeometryGenerator::Plane(XERDevice* device, XERGeometry* geometry)
{
	static VertexBase vertices[] =
	{
		{ { -1.0f, 0.0f, -1.0f, },{ 0.0f, 1.0f, 0.0f } },
		{ { 1.0f, 0.0f, 1.0f, },{ 0.0f, 1.0f, 0.0f } },
		{ { -1.0f, 0.0f, 1.0f, },{ 0.0f, 1.0f, 0.0f } },
		{ { 1.0f, 0.0f, -1.0f, },{ 0.0f, 1.0f, 0.0f } },
	};

	static uint32 indices[] =
	{
		0, 2, 1,
		0, 1, 3,
	};

	geometry->initialize(device, vertices, countof(vertices), sizeof(VertexBase), indices, countof(indices));
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

void XERGeometryGenerator::Sphere(XERDevice* device, XERGeometry* geometry)
{
	constexpr uint32 subdivisionCount = 1;
	//constexpr uint32 vertexPerFaceCount = 
}

void XERGeometryGenerator::MonsterSkinned(XERDevice* device, XERGeometry* geometry)
{
	geometry->initialize(device, monsterSkinnedVertices, countof(monsterSkinnedVertices),
		sizeof(monsterSkinnedVertices[0]), monsterIndices, countof(monsterIndices));
}