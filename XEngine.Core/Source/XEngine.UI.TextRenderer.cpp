#include <XLib.Util.h>
#include <XLib.Vectors.Arithmetics.h>
#include <XEngine.Render.UI.Batch.h>
#include <XEngine.Render.Vertices.h>

#include "XEngine.UI.TextRenderer.h"

#include "XEngine.UI.MonospacedFont.h"

using namespace XEngine::UI;
using namespace XEngine::Render;
using namespace XEngine::Render::UI;

void TextRenderer::beginDraw(Batch& batch)
{
	this->batch = &batch;


}
void TextRenderer::endDraw()
{
	
}

void TextRenderer::setPosition(float32x2 position)
{
	this->position = position * batch->getNDCScale() + float32x2(-1.0f, 1.0f);
	left = this->position.x;
}

void TextRenderer::setFont(MonospacedFont& font)
{
	batch->setTexture(font.getTexture());
	characterSize = float32x2(font.getCharWidth(), font.getCharHeight()) * batch->getNDCScale();
	firstCharCode = font.getFirstCharCode();
	charTableSize = font.getCharTableSize();
}

void TextRenderer::setColor(XLib::Color color)
{
	this->color = color;
}

void TextRenderer::write(const char* string, uint32 length)
{
	uint8 lastCharCode = firstCharCode + charTableSize - 1;

	const char* limit = string + length;
	for (const char *i = string; i != limit; i++)
	{
		char character = *i;

		if (character == '\0')
			break;

		if (character == '\n')
		{
			position.x = left;
			position.y += characterSize.y;
			continue;
		}
		
		if (character == ' ' || character < firstCharCode || character >= lastCharCode)
		{
			position.x = left;
			continue;
		}

		character -= firstCharCode;
		float32 bottom = position.y + characterSize.y;

		uint16 charTextureLeft = uint16(0xFFFF * character / charTableSize);
		VertexUIColorTexture leftTop = { position, color, { charTextureLeft, 0 }};
		VertexUIColorTexture leftBottom = { { position.x, bottom }, color, { charTextureLeft, 0xFFFF } };

		position.x += characterSize.x;
		uint16 charTextureRight = uint16(0xFFFF * (character + 1) / charTableSize);
		VertexUIColorTexture rightTop = { position, color, { charTextureRight, 0 } };
		VertexUIColorTexture rightBottom = { { position.x, bottom }, color, { charTextureRight, 0xFFFF } };

		VertexUIColorTexture *vertices = 
			to<VertexUIColorTexture*>(batch->allocateVertices(GeometryType::ColorTexture, 6));

		vertices[0] = leftTop;
		vertices[1] = rightTop;
		vertices[2] = leftBottom;
		vertices[3] = rightTop;
		vertices[4] = rightBottom;
		vertices[5] = leftBottom;
	}
}