#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Color.h>

namespace XEngine::Render::UI { class Batch; }
namespace XEngine::UI { class MonospacedFont; }

namespace XEngine::UI
{
	class TextRenderer : public XLib::NonCopyable
	{
	private:
		Render::UI::Batch *batch = nullptr;
		float32x2 position = { 0.0f, 0.0f };
		float32x2 characterSize = { 0.0f, 0.0f };
		float32 left = 0.0f;
		XLib::Color color = 0;
		uint8 firstCharCode = 0;
		uint8 charTableSize = 0;

	public:
		TextRenderer() = default;
		~TextRenderer() = default;

		void beginDraw(Render::UI::Batch& batch);
		void endDraw();

		void setPosition(float32x2 position);
		void setFont(MonospacedFont& font);
		void setColor(XLib::Color color);

		void write(const char* string, uint32 length = uint32(-1));
	};
}