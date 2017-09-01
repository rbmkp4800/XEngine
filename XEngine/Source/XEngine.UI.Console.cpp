#include "XEngine.UI.Console.h"
#include "XEngine.Render.UIRender.h"
#include "XEngine.Render.Vertices.h"
#include "XEngine.Render.Resources.h"
#include "XEngine.Color.h"

using namespace XLib;
using namespace XEngine;

void XEUIConsole::initialize(XERDevice* device, XERMonospacedFont* font)
{
	this->font = font;
	buffer.resize(4096);
	currentCommandLength = 0;
}

void XEUIConsole::handleCharacter(wchar character)
{
	if (character == 0x1B) // escape
	{
		currentCommandLength = 0;
		return;
	}
	if (character == 0x08) // backspace
	{
		if (currentCommandLength > 0)
			currentCommandLength--;
		return;
	}
	else if (character == 0x0D) // enter
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

	if (character >= ' ' && character < 127)
		buffer[currentCommandLength++] = char(character);
}

void XEUIConsole::draw(XERUIRender* uiRender)
{
	VertexUIColor *vertices = to<VertexUIColor*>(uiRender->allocateVertices(
		sizeof(VertexUIColor) * 6, XERUIGeometryType::Color));

	uint32 color = 0x101010E8_rgba;
	vertices[0] = { { 1.0f, 1.0f }, color };
	vertices[1] = { { 1.0f, 0.0f }, color };
	vertices[2] = { { -1.0f, 0.0f }, color };
	vertices[3] = { { 1.0f, 1.0f }, color };
	vertices[4] = { { -1.0f, 0.0f }, color };
	vertices[5] = { { -1.0f, 1.0f }, color };

	buffer[currentCommandLength] = '_';

	uiRender->drawText(font, { 0.0f, 0.0f }, ">>>", 0xFFFFFF_rgb, 3);
	uiRender->drawText(font, { float32(font->getCharWidth() * 4), 0.0f },
		buffer, 0x00FFFF_rgb, currentCommandLength + 1);
	uiRender->drawText(font, { 0.0f, float32(font->getCharHeight()) },
		"XEngine console sample text\nOne more line\n * ASDASDASD\n * BDHBDHBCHDBCh", 0x808080_rgb);
	uiRender->drawText(font, { 0.0f, float32(font->getCharHeight() * 5) },
		"jancjsdkncjkdsncjkNKJANDJKASNDKJSANDJKASNJKD\nkandjandjkandjkasn", 0xFFFFFF_rgb);
	uiRender->drawText(font, { 0.0f, float32(font->getCharHeight() * 7) }, "========= LOADING COMPLETE =========", 0x00B000_rgb);
	uiRender->drawText(font, { 0.0f, float32(font->getCharHeight() * 8) }, "21:05:14.654 [ERROR] some error message", 0xC00000_rgb);
	uiRender->drawText(font, { 0.0f, float32(font->getCharHeight() * 9) }, "21:05:14.841 [WARNING] some warning message", 0xC0C000_rgb);
}