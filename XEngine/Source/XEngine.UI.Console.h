#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Heap.h>
#include <XLib.Delegate.h>

namespace XEngine
{
	class XERMonospacedFont;
	class XERUIRender;
	class XERDevice;

	using XEUIConsoleCommandHandler = XLib::Delegate<void, const char* /*command*/>;

	class XEUIConsole : public XLib::NonCopyable
	{
	private:
		XERMonospacedFont *font;

		XLib::HeapPtr<char> buffer;
		uint16 currentCommandLength;
		//Internal::UIGeometryBuffer gpuGeometryBuffer;

		XEUIConsoleCommandHandler handler;

	public:
		void initialize(XERDevice* device, XERMonospacedFont* font);

		void handleCharacter(wchar character);
		void draw(XERUIRender* uiRender);

		inline void setCommandHandler(XEUIConsoleCommandHandler handler) { this->handler = handler; }
	};
}