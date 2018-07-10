#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.Render.Base.h"

namespace XEngine::Render::Device_
{
	class TextureHeap : public XLib::NonCopyable
	{
	public:
		TextureHeap() = default;
		~TextureHeap() = default;
	};
}