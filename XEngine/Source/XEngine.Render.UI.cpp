#include <d3d12.h>
#include "Util.D3D12.h"

#include <XLib.Util.h>
#include <XLib.Heap.h>
#include <XLib.Vectors.Arithmetics.h>

#include "XEngine.Render.UI.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Vertices.h"

using namespace XLib;
using namespace XEngine;

static constexpr uint32 vertexBufferByteSize = 0x8000;

void XERUIGeometryRenderer::flush()
{

}

void XERUIGeometryRenderer::fillD3DCommandList(ID3D12GraphicsCommandList *d3dCommandList)
{
	d3dCommandList->SetPipelineState(device->d3dUIFontPSO);
	d3dCommandList->SetGraphicsRootDescriptorTable(0, device->srvHeap.getGPUHandle(font->srvDescriptor));
	d3dCommandList->IASetVertexBuffers(0, 1,
		&D3D12VertexBufferView(d3dVertexBuffer->GetGPUVirtualAddress(),
		usedVertexCount * sizeof(VertexUIFont), sizeof(VertexUIFont)));
	d3dCommandList->DrawInstanced(usedVertexCount, 1, 0, 0);
}

void XERUIGeometryRenderer::initialize(XERDevice *device)
{
	this->device = device;

	ID3D12Device *d3dDevice = device->d3dDevice;
	d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(vertexBufferByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dVertexBuffer.uuid(), d3dVertexBuffer.voidInitRef());
	d3dVertexBuffer->Map(0, &D3D12Range(), to<void**>(&mappedVertexBuffer));
}

void XERUIGeometryRenderer::beginDraw(XERContext* context)
{
	this->context = context;
	usedVertexCount = 0;
}

void XERUIGeometryRenderer::endDraw()
{
	flush();
	context = nullptr;
}

void XERUIGeometryRenderer::drawText(float32x2 position, const char* text, uint32 length)
{
	uint8 firstCharCode = font->firstCharCode;
	uint8 lastCharCode = firstCharCode + font->charTableSize - 1;
	uint32 charTableSize = font->charTableSize;

	position *= ndcScaleCoef;
	position += float32x2(-1.0f, 1.0f);
	float32x2 charSize = float32x2(font->charWidth, font->charHeight) * ndcScaleCoef;
	float32 leftBorder = position.x;

	VertexUIFont *vertices = to<VertexUIFont*>(mappedVertexBuffer) + usedVertexCount;
	for (uint32 i = 0; i < length; i++)
	{
		char character = text[i];

		if (character == '\0')
			break;

		if (character == '\n')
		{
			position.x = leftBorder;
			position.y += charSize.y;
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
			VertexUIFont leftTop = { position, { charTextureLeft, 0 } };
			VertexUIFont leftBottom = { { position.x, bottom }, { charTextureLeft, 0xFFFF } };

			position.x += charSize.x;
			uint16 charTextureRight = uint16(0xFFFF * (character + 1) / charTableSize);
			VertexUIFont rightTop = { position,{ charTextureRight, 0 } };
			VertexUIFont rightBottom = { { position.x, bottom },{ charTextureRight, 0xFFFF } };

			vertices[0] = leftTop;
			vertices[1] = rightTop;
			vertices[2] = leftBottom;
			vertices[3] = rightTop;
			vertices[4] = rightBottom;
			vertices[5] = leftBottom;

			vertices += 6;
			usedVertexCount += 6;
		}
	}
}

// XERMonospacedFont ========================================================================//

/*void XERMonospacedFont::initializeA8(XERDevice* device, uint8* bitmapA8,
	uint8 charWidth, uint8 charHeight, uint8 firstCharCode, uint8 charTableSize)
{
	this->charWidth = charWidth;
	this->charHeight = charHeight;
	this->firstCharCode = firstCharCode;
	this->charTableSize = charTableSize;
	uint32 textureWidth = uint32(charWidth) * uint32(charTableSize);

	device->d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &D3D12ResourceDesc_Texture2D(DXGI_FORMAT_A8_UNORM, textureWidth, charHeight),
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, d3dTexture.uuid(), d3dTexture.voidInitRef());
}*/

void XERMonospacedFont::initializeA1(XERDevice* device, uint8* bitmapA1,
	uint8 charWidth, uint8 charHeight, uint8 firstCharCode, uint8 charTableSize)
{
	this->charWidth = charWidth;
	this->charHeight = charHeight;
	this->firstCharCode = firstCharCode;
	this->charTableSize = charTableSize;
	uint32 textureWidth = uint32(charWidth) * uint32(charTableSize);

	uint32 bitmapSize = textureWidth * charHeight;
	HeapPtr<uint8> bitmap(bitmapSize);
	for (uint32 i = 0; i < bitmapSize; i++)
		bitmap[i] = (bitmapA1[i / 8] >> (i % 8)) & 1 ? 0xFF : 0;

	device->d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &D3D12ResourceDesc_Texture2D(DXGI_FORMAT_A8_UNORM, textureWidth, charHeight),
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, d3dTexture.uuid(), d3dTexture.voidInitRef());

	srvDescriptor = device->srvHeap.allocateDescriptors(1);
	device->d3dDevice->CreateShaderResourceView(d3dTexture, nullptr, device->srvHeap.getCPUHandle(srvDescriptor));
	device->uploadEngine.uploadTexture(DXGI_FORMAT_A8_UNORM, d3dTexture, bitmap, textureWidth, charHeight);
}