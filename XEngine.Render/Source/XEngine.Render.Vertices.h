#pragma once

#include <XLib.Types.h>
#include <XLib.Vectors.h>

// TODO: encode normal and tangent as r10g10b10a2_unorm

namespace XEngine::Render
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

	struct VertexTangentTexture
	{
		float32x3 position;
		float32x3 normal;
		float32x3 tangent;
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

	struct VertexUIColorTexture
	{
		float32x2 position;
		uint32 color;
		uint16x2 texture;
	};
}