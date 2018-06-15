#pragma once

#include <XLib.Types.h>

namespace XEngine::Render::Internal
{
	struct RootSignaturesConfig abstract final
	{
		struct GBufferPass abstract final
		{
			static constexpr uint32
				TransformCBVIndex = ,
				MaterialCBVIndex = ;
		};
	};
}