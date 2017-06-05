#pragma once

#include <d3d12.h>

#include <XLib.Types.h>

namespace XEngine::Internal
{
	struct GPUDefaultDrawingIC
	{
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

		// non command data
		uint32 geometryInstanceId;
	};

	struct GPUGeometryInstanceDesc
	{
		uint32 drawCommandId;
		uint32 transformsBaseOffset;
	};

	/*struct GPUxOCBBoxDesc
	{
		XLib::Matrix3x4 transform;
		uint32 geometryInstanceId;	// to set visibility flag
		uint32 geometryInstanceTransformBaseOffset;
	};*/
}