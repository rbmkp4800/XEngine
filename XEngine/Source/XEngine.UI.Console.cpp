#include <d3d12.h>
#include "Util.D3D12.h"

#include "XEngine.UI.Console.h"
#include "XEngine.Render.UIRender.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Vertices.h"
#include "XEngine.Color.h"

using namespace XLib;
using namespace XEngine;

void XEUIConsole::initialize(XERDevice* device, XERMonospacedFont* font)
{
	this->font = font;
	buffer.resize(4096);
	currentCommandLength = 0;
}

void XEUIConsole::handleCharacter(wchar key)
{
	if (key == 0x1B) // escape
		currentCommandLength = 0;
	if (key == 0x08) // backspace
	{
		if (currentCommandLength > 0)
			currentCommandLength--;
		return;
	}
	else if (key == 0x0D) // enter
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
	VertexUIColor *vertices = to<VertexUIColor*>(uiRender->allocateVertexBuffer(
		sizeof(VertexUIColor) * 6, XERUIRender::GeometryType::Color));

	uint32 color = 0x181818D0_rgba;
	vertices[0] = { { 1.0f, 1.0f }, color };
	vertices[1] = { { 1.0f, 0.0f }, color };
	vertices[2] = { { -1.0f, 0.0f }, color };
	vertices[3] = { { 1.0f, 1.0f }, color };
	vertices[4] = { { -1.0f, 0.0f }, color };
	vertices[5] = { { -1.0f, 1.0f }, color };

	uiRender->drawText(font, { 0.0f, 0.0f }, buffer, currentCommandLength);
}