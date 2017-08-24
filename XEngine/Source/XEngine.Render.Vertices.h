#pragma once

#include <XLib.Types.h>
#include <XLib.Vectors.h>

namespace XEngine
{
	struct VertexBase
	{
		float32x3 position;
		float32x3 normal;
	};

	struct VertexTexture
	{
		float32x3 position;
		float32x3 normal;
		float32x2 texture;
	};

	struct VertexBaseSkinned
	{
		float32x3 position;
		float32x3 normal;
		uint8x4 jointWeights;
		uint16x4 jointIndices;
	};

	struct VertexUIColor
	{
		float32x2 position;
		uint32 color;
	};

	struct VertexUIFont
	{
		float32x2 position;
		uint16x2 texture;
		uint32 color;
	};
}