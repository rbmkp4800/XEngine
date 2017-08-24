#include <d3d12.h>
#include "Util.D3D12.h"

#include "XEngine.UI.Console.h"
#include "XEngine.Render.UIRender.h"
#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine;

void XEUIConsole::initialize(XERDevice* device, XERMonospacedFont* font)
{
	this->font = font;
	buffer.resize(4096);
	currentCommandLength = 0;
}

void XEUIConsole::handleKeyboard(XLib::VirtualKey key)
{
	if (key == VirtualKey::Backspace)
	{
		if (currentCommandLength > 0)
			currentCommandLength--;
		return;
	}
	else if (key == VirtualKey(0x0D)) // TODO: refactor
	{
		if (currentCommandLength > 0)
		{
			if (handler.isInitialized())
			{
				buffer[currentCommandLength] = 0;
				handler.call(buffer);
			}
			currentCommandLength = 0;
		}
		return;
	}

	char character = char(key);
	if (character >= ' ' || character < 127)
		buffer[currentCommandLength++] = (char)key;
}

void XEUIConsole::draw(XERUIRender* uiRender)
{
	uiRender->drawText(font, { 0.0f, 0.0f }, buffer, currentCommandLength);
}