#pragma once

#include <d3d12.h>

#include <XLib.Types.h>

namespace XEngine::Internal
{
	struct GPUDefaultDrawingIndirectCommand
	{
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};

	struct GPUGeometryInstanceDesc
	{
		uint32 drawCommandId;
		uint32 transformsIndex;
	};
}