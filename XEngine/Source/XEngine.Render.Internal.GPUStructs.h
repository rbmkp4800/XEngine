#pragma once

#include <d3d12.h>

#include <XLib.Types.h>

#pragma pack(push, 4)
namespace XEngine::Internal
{
	struct GPUDefaultDrawingIC
	{
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;

		// instance constants
		uint32 textureId;
		//uint32 baseTransformIndex;

		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

		// non command data
		uint32 geometryInstanceId;
	};

	struct GPUGeometryInstanceDesc
	{
		uint32 drawCommandId;
		uint32 transformsBaseOffset;
	};

	struct GPUxBVHNode
	{
		uint32 isLeafFlag_parentId;
		union
		{
			struct // internal node
			{
				uint32 leftChildId;
				uint32 rightChildId;
			};

			struct // leaf node
			{
				uint32 geometryInstanceId;
				uint32 drawCommandId;
			};
		};

		float32x3 bboxMin;
		float32x3 bboxMax;
	};
}
#pragma pack(pop)