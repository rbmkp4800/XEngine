#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Heap.h>
#include <XLib.Delegate.h>

#include "XEngine.Render.Resources.h"

#define XECONSOLE_SETCOLOR_WHITE_LITERAL	"\x1B\xFF"
#define XECONSOLE_SETCOLOR_RED_LITERAL		"\x1B\xC3"
#define XECONSOLE_SETCOLOR_GREEN_LITERAL	"\x1B\xCC"
#define XECONSOLE_SETCOLOR_BLUE_LITERAL		"\x1B\xF0"
#define XECONSOLE_SETCOLOR_YELLOW_LITERAL	"\x1B\xCF"
#define XECONSOLE_SETCOLOR_CYAN_LITERAL		"\x1B\xFC"
#define XECONSOLE_SETCOLOR_MAGENTA_LITERAL	"\x1B\xF3"
#define XECONSOLE_SETCOLOR_DARKGRAY_LITERAL	"\x1B\xD5"
#define XECONSOLE_SETCOLOR_LIGHTGRAY_LITERAL "\x1B\xEA"

namespace XEngine
{
	class XERMonospacedFont;
	class XERUIRender;
	class XERDevice;

	using XEUIConsoleCommandHandler = XLib::Delegate<void, const char* /*command*/>;

	class XEUIConsole : public XLib::NonCopyable
	{
	private:
		static constexpr uint32 textBufferSizeLog2 = 14;
		static constexpr uint32 textBufferSize = 1 << textBufferSizeLog2;
		static constexpr uint32 textBufferIndexMask = textBufferSize - 1;
		static constexpr uint32 textBufferNotIndexMask = ~textBufferIndexMask;

		//===================================================================================//

		XERMonospacedFont *font = nullptr;

		XLib::HeapPtr<char> textBuffer;

		uint32 textHeadCyclicIndex = 0;
		uint32 textTailCyclicIndex = 0;

		XERUIGeometryBuffer textGeometryBuffer;
		uint32 textGeometryBufferLength = 0;
		uint32 textGeometryBufferCyclicIndex = 0;

		uint16 rowCount = 0, columnCount = 0;

		char commandBuffer[256];
		uint16 commandLength = 0;
		XEUIConsoleCommandHandler handler;

	public:
		void initialize(XERDevice* device, XERMonospacedFont* font);

		void write(const char* message, uint32 length = uint32(-1));
		void draw(XERUIRender* uiRender);

		void handleCharacter(wchar character);

		inline void setCommandHandler(XEUIConsoleCommandHandler handler) { this->handler = handler; }
	};
}