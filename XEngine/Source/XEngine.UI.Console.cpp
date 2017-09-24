#include <XLib.Memory.h>
#include <XLib.Vectors.h>
#include <XLib.Vectors.Arithmetics.h>
#include <XLib.Strings.h>

#include "XEngine.UI.Console.h"
#include "XEngine.Render.UIRender.h"
#include "XEngine.Render.Vertices.h"
#include "XEngine.Render.Resources.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Color.h"

using namespace XLib;
using namespace XEngine;

static constexpr uint32 backgroundColor = 0x101010E8_rgba;

namespace
{
	struct CharacterGeometry
	{
		VertexUIFont aLeftTop;
		VertexUIFont aRightTop;
		VertexUIFont aLeftBottom;
		VertexUIFont bRightTop;
		VertexUIFont bRightBottom;
		VertexUIFont bLeftBottom;
	};
}

void XEUIConsole::initialize(XERDevice* device, XERMonospacedFont* font)
{
	this->font = font;

	textBuffer.resize(textBufferSize);

	textGeometryBufferLength = 1024;
	textGeometryBuffer.initialize(device, textGeometryBufferLength * sizeof(CharacterGeometry));
}

void XEUIConsole::write(const char* message, uint32 length)
{
	// TODO: refactor. Copy message while counting length
	if (length == uint32(-1))
		length = Strings::Length(message);

	if (length > textBufferSize)
	{
		message += textBufferSize - length;
		length = textBufferSize;
	}

	uint32 textHeadIndex = (textHeadCyclicIndex & textBufferIndexMask);
	if ((textHeadIndex + length) & textBufferNotIndexMask)
	{
		uint32 firstPartSize = textBufferSize - textHeadIndex;
		uint32 secondPartSize = length - firstPartSize;

		Memory::Copy(textBuffer + textHeadIndex, message, firstPartSize);
		Memory::Copy(textBuffer, message + firstPartSize, secondPartSize);
	}
	else
		Memory::Copy(textBuffer + textHeadIndex, message, length);

	textHeadCyclicIndex += length;
	if (textHeadCyclicIndex - textTailCyclicIndex > textBufferSize)
	{
		textTailCyclicIndex = textHeadCyclicIndex - textBufferSize;

		// TODO: fix for uint32 overflow support
		//while (!linesQueue.isEmpty() && linesQueue.front().textCyclicIndex < textTailCyclicIndex)
		//	linesQueue.dropFront();
	}
}

void XEUIConsole::draw(XERUIRender* uiRender)
{
	rowCount = uiRender->getTargetHeight() / 2 / font->getCharHeight();

	// dawing background

	VertexUIColor *vertices = to<VertexUIColor*>(uiRender->allocateVertices(
		sizeof(VertexUIColor) * 6, XERUIGeometryType::Color));
	float32 bottom = 1.0f + float32((rowCount + 1 /* + command line row */)
		* font->getCharHeight()) * uiRender->getNDCVerticalScale();
	vertices[0] = { { 1.0f, 1.0f }, backgroundColor };
	vertices[1] = { { 1.0f, bottom }, backgroundColor };
	vertices[2] = { { -1.0f, bottom }, backgroundColor };
	vertices[3] = { { 1.0f, 1.0f }, backgroundColor };
	vertices[4] = { { -1.0f, bottom }, backgroundColor };
	vertices[5] = { { -1.0f, 1.0f }, backgroundColor };

	// drawing command line

	commandBuffer[commandLength] = '_';
	float32 commandLineTop = rowCount * font->getCharHeight();
	float32 commandLineLeft = font->getCharWidth() * 4;
	uiRender->drawText(font, float32x2(0.0f, commandLineTop), ">>>", 0xFFFFFF_rgb, 3);
	uiRender->drawText(font, float32x2(commandLineLeft, commandLineTop), commandBuffer, 0x00FFFF_rgb, commandLength + 1);

	// drawing history text

	if (textHeadCyclicIndex == textTailCyclicIndex)
		return;

	//if (linesQueue.isEmpty())
	{
		// looking for top visible line (iterating backwards)

		uint32 lineCount = 0;
		char *head = textBuffer + (textHeadCyclicIndex & textBufferIndexMask);
		char *tail = textBuffer + (textTailCyclicIndex & textBufferIndexMask);
		char *current;
		if ((textHeadCyclicIndex ^ textTailCyclicIndex) & textBufferNotIndexMask)
		{
			// text crosses textBuffer bound

			for (current = head - 1; current >= textBuffer; current--)
			{
				if (*current == '\n')
				{
					lineCount++;
					if (lineCount >= rowCount)
						goto label_topLineFound;
				}
			}
			for (current = textBuffer + textBufferSize - 1; current >= tail; current--)
			{
				if (*current == '\n')
				{
					lineCount++;
					if (lineCount >= rowCount)
						goto label_topLineFound;
				}
			}
		}
		else
		{
			for (current = head - 1; current >= tail; current--)
			{
				if (*current == '\n')
				{
					lineCount++;
					if (lineCount >= rowCount)
						goto label_topLineFound;
				}
			}
		}
		lineCount++;

	label_topLineFound:
		current++;

		// generating geometry from scratch

		float32x2 position = { -1.0f, 1.0f };
		float32x2 charSize = float32x2(font->getCharWidth(), font->getCharHeight());
		charSize *= uiRender->getNDCScale();

		uint8 firstCharCode = font->getFirstCharCode();
		uint8 charTableSize = font->getCharTableSize();
		uint8 lastCharCode = firstCharCode + charTableSize - 1;

		bool setColorSequence = false;
		uint32 color = 0xFFFFFFFF;

		uint32 characterCount = 0;

		while (current != head)
		{
			char character = *current;

			if (setColorSequence)
			{
				XEColor tempColor;
				uint8 p;
				p = character & 0b00000011; tempColor.r = (p << 6) | (p << 4) | (p << 2) | p;
				p = character & 0b00001100; tempColor.g = (p << 4) | (p << 2) | p | (p >> 2);
				p = character & 0b00110000; tempColor.b = (p << 2) | p | (p >> 2) | (p >> 4);
				p = character & 0b11000000; tempColor.a = p | (p >> 2) | (p >> 4) | (p >> 6);

				color = tempColor;
				setColorSequence = false;
			}
			else if (character == '\n')
			{
				position.x = -1.0f;
				position.y += charSize.y;
				color = 0xFFFFFFFF;
			}
			else if (character == '\x1B')
			{
				setColorSequence = true;
			}
			else if (character == ' ' || character < firstCharCode || character >= lastCharCode)
			{
				position.x += charSize.x;
			}
			else
			{
				character -= firstCharCode;
				float32 bottom = position.y + charSize.y;

				uint16 charTextureLeft = uint16(0xFFFF * character / charTableSize);
				VertexUIFont leftTop = { position,{ charTextureLeft, 0 }, color };
				VertexUIFont leftBottom = { { position.x, bottom },{ charTextureLeft, 0xFFFF }, color };

				position.x += charSize.x;
				uint16 charTextureRight = uint16(0xFFFF * (character + 1) / charTableSize);
				VertexUIFont rightTop = { position,{ charTextureRight, 0 }, color };
				VertexUIFont rightBottom = { { position.x, bottom },{ charTextureRight, 0xFFFF }, color };

				CharacterGeometry geometry;
				geometry.aLeftTop = leftTop;
				geometry.aRightTop = rightTop;
				geometry.aLeftBottom = leftBottom;
				geometry.bRightTop = rightTop;
				geometry.bRightBottom = rightBottom;
				geometry.bLeftBottom = leftBottom;

				textGeometryBuffer.update(uiRender->getDevice(),
					characterCount * sizeof(CharacterGeometry),
					&geometry, sizeof(CharacterGeometry));

				characterCount++;
				if (characterCount >= textGeometryBufferLength)
					break;
			}

			current++;
			if (current >= textBuffer + textBufferSize)
				current = textBuffer;
		}

		uiRender->drawBuffer(&textGeometryBuffer, 0, characterCount * sizeof(CharacterGeometry), XERUIGeometryType::Font);
	}
}

void XEUIConsole::handleCharacter(wchar character)
{
	if (character == 0x1B) // escape
	{
		commandLength = 0;
		return;
	}
	if (character == 0x08) // backspace
	{
		if (commandLength > 0)
			commandLength--;
		return;
	}
	else if (character == 0x0D) // enter
	{
		if (commandLength > 0)
		{
			if (handler.isInitialized())
			{
				commandBuffer[commandLength] = '\n';
				write(commandBuffer, commandLength + 1);
				commandBuffer[commandLength] = '\0';
				handler.call(commandBuffer);
			}
			commandLength = 0;
		}
		return;
	}

	if (character >= ' ' && character < 127)
		commandBuffer[commandLength++] = char(character);
}