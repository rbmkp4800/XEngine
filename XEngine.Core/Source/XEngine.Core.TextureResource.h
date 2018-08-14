#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XEngine.Render.Base.h>

namespace XEngine::Core
{
	class TextureResource : public XLib::NonCopyable
	{
	private:
		Render::TextureHandle texture = Render::TextureHandle(0);

	public:
		TextureResource() = default;
		~TextureResource() = default;

		bool createFromFile(const char* filename);

		inline Render::TextureHandle getTexture() const { return texture; }
	};
}