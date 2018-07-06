#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.Render.Base.h"

namespace XEngine::Render::Device_
{
	class TextureHeap final : public XLib::NonCopyable
	{
		friend Device;

	private:
		TextureHeap() = default;
		~TextureHeap() = default;

		inline Device* getDevice();

	public:

	};
}