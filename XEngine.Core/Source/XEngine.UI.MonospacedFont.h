#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XEngine.Render.UI.Resources.h>

namespace XEngine::UI
{
	class MonospacedFont : public XLib::NonCopyable
	{
	private:
		Render::UI::Texture texture;
		uint8 charWidth = 0, charHeight = 0;
		uint8 firstCharCode = 0, charTableSize = 0;

	public:
		MonospacedFont() = default;
		~MonospacedFont() = default;

		void initializeFromA1Bitmap();

		inline Render::UI::Texture& getTexture() { return texture; }
		inline uint8 getCharWidth() const { return charWidth; }
		inline uint8 getCharHeight() const { return charHeight; }
		inline uint8 getFirstCharCode() const { return firstCharCode; }
		inline uint8 getCharTableSize() const { return charTableSize; }
	};
}