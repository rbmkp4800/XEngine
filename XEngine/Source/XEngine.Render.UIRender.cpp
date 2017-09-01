#include <d3d12.h>
#include "Util.D3D12.h"

#include <XLib.Util.h>
#include <XLib.Vectors.Arithmetics.h>

#include "XEngine.Render.UIRender.h"
#include "XEngine.Render.Resources.h"
#include "XEngine.Render.Targets.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Vertices.h"
#include "XEngine.Color.h"

using namespace XLib;
using namespace XEngine;

static constexpr uint32 vertexBufferSize = 0x8000;

// private ==================================================================================//

void XERUIRender::drawVertexBuffer(ID3D12Resource *d3dVertexBufferToDraw,
	uint32 offset, uint32 size, XERUIGeometryType geometryType)
{
	if (!commandListInitialized)
		initCommandList();

	ID3D12PipelineState *d3dPS = nullptr;
	uint32 vertexStride = 0;
	bool srvNeeded = false;
	switch (currentGeometryType)
	{
		case XERUIGeometryType::Color:
			vertexStride = sizeof(VertexUIColor);
			d3dPS = device->d3dUIColorPSO;
			break;

		case XERUIGeometryType::Font:
			vertexStride = sizeof(VertexUIFont);
			d3dPS = device->d3dUIFontPSO;
			srvNeeded = true;
			break;

		default: throw;
	}

	d3dCommandList->SetPipelineState(d3dPS);
	if (srvNeeded && currentGeometrySRVDescriptor != uint32(-1))
		d3dCommandList->SetGraphicsRootDescriptorTable(0, device->srvHeap.getGPUHandle(currentGeometrySRVDescriptor));
	d3dCommandList->IASetVertexBuffers(0, 1, &D3D12VertexBufferView(d3dVertexBufferToDraw->GetGPUVirtualAddress() + offset, size, vertexStride));
	d3dCommandList->DrawInstanced(vertexBufferSize / vertexStride, 1, 0, 0);
}

void XERUIRender::flushCurrentGeometry()
{
	if (currentGeometryVertexBufferOffset == vertexBufferUsedBytes)
		return;

	drawVertexBuffer(d3dVertexBuffer, currentGeometryVertexBufferOffset,
		vertexBufferUsedBytes - currentGeometryVertexBufferOffset, currentGeometryType);

	currentGeometryVertexBufferOffset = vertexBufferUsedBytes;
	currentGeometryType = XERUIGeometryType::None;
}

void XERUIRender::flushCommandList()
{
	flushCurrentGeometry();

	if (!commandListInitialized)
		return;

	d3dCommandList->Close();
	ID3D12CommandList *d3dCommandLists[] = { d3dCommandList };
	device->graphicsGPUQueue.execute(d3dCommandLists, countof(d3dCommandLists));

	commandListInitialized = false;
	vertexBufferUsedBytes = 0;
	currentGeometryVertexBufferOffset = 0;
}

void XERUIRender::initCommandList()
{
	d3dCommandAllocator->Reset();
	d3dCommandList->Reset(d3dCommandAllocator, nullptr);

	d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, float32(targetWidth), float32(targetHeight)));
	d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, targetWidth, targetHeight));

	d3dCommandList->SetGraphicsRootSignature(device->d3dUIPassRS);
	ID3D12DescriptorHeap *d3dDescriptorHeaps[] = { device->srvHeap.getD3D12DescriptorHeap() };
	d3dCommandList->SetDescriptorHeaps(countof(d3dDescriptorHeaps), d3dDescriptorHeaps);

	D3D12_CPU_DESCRIPTOR_HANDLE targetRTVDescriptorHandle = device->rtvHeap.getCPUHandle(targetRTVDescriptor);
	d3dCommandList->OMSetRenderTargets(1, &targetRTVDescriptorHandle, FALSE, nullptr);

	commandListInitialized = true;
}

// public ===================================================================================//

void* XERUIRender::allocateVertices(uint32 size, XERUIGeometryType geometryType)
{
	if (vertexBufferUsedBytes + size > vertexBufferSize)
		flushCommandList();

	if (geometryType != currentGeometryType)
	{
		flushCurrentGeometry();
		currentGeometryType = geometryType;
		currentGeometryVertexBufferOffset = vertexBufferUsedBytes;
	}

	void *result = to<byte*>(mappedVertexBuffer) + vertexBufferUsedBytes;
	vertexBufferUsedBytes += size;
	return result;
}

void XERUIRender::drawBuffer(XERUIGeometryBuffer* buffer,
	uint32 offset, uint32 size, XERUIGeometryType geometryType)
{
	flushCurrentGeometry();
	drawVertexBuffer(buffer->d3dBuffer, offset, size, geometryType);
}

void XERUIRender::setTexture(XERTexture* texture)
{
	if (currentGeometrySRVDescriptor != texture->srvDescriptor)
	{
		flushCurrentGeometry();
		currentGeometrySRVDescriptor = texture->srvDescriptor;
	}
}

void XERUIRender::setTexture(XERMonospacedFont* fontTexture)
{
	if (currentGeometrySRVDescriptor != fontTexture->srvDescriptor)
	{
		flushCurrentGeometry();
		currentGeometrySRVDescriptor = fontTexture->srvDescriptor;
	}
}

void XERUIRender::initialize(XERDevice *device)
{
	this->device = device;

	ID3D12Device *d3dDevice = device->d3dDevice;
	d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dVertexBuffer.uuid(), d3dVertexBuffer.voidInitRef());
	d3dVertexBuffer->Map(0, &D3D12Range(), to<void**>(&mappedVertexBuffer));
}

void XERUIRender::beginDraw(XERTargetBuffer* target)
{
	if (!device->graphicsCommandListPool.acquireDefault(d3dCommandList, d3dCommandAllocator))
		return;

	D3D12_RESOURCE_DESC desc = target->d3dTexture->GetDesc();
	targetWidth = uint16(desc.Width);
	targetHeight = uint16(desc.Height);
	targetRTVDescriptor = target->rtvDescriptor;
}

void XERUIRender::endDraw()
{
	flushCommandList();

	device->graphicsCommandListPool.releaseDefault();
	d3dCommandList = nullptr;
	d3dCommandAllocator = nullptr;
	targetWidth = 0;
	targetHeight = 0;
	targetRTVDescriptor = 0;
}

// utils ====================================================================================//

void XERUIRender::drawText(XERMonospacedFont* font, float32x2 position,
	const char* text, uint32 color, uint32 length)
{
	setTexture(font);

	uint8 firstCharCode = font->firstCharCode;
	uint8 lastCharCode = firstCharCode + font->charTableSize - 1;
	uint32 charTableSize = font->charTableSize;

	float32x2 ndcScaleCoef = getNDCScaleCoef();

	position *= ndcScaleCoef;
	position += float32x2(-1.0f, 1.0f);
	float32x2 charSize = float32x2(font->charWidth, font->charHeight) * ndcScaleCoef;
	float32 leftBorder = position.x;

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
			VertexUIFont leftTop = { position, { charTextureLeft, 0 }, color };
			VertexUIFont leftBottom = { { position.x, bottom }, { charTextureLeft, 0xFFFF }, color };

			position.x += charSize.x;
			uint16 charTextureRight = uint16(0xFFFF * (character + 1) / charTableSize);
			VertexUIFont rightTop = { position,{ charTextureRight, 0 }, color };
			VertexUIFont rightBottom = { { position.x, bottom },{ charTextureRight, 0xFFFF }, color };

			VertexUIFont *vertices = to<VertexUIFont*>(allocateVertices(sizeof(VertexUIFont) * 6, XERUIGeometryType::Font));

			vertices[0] = leftTop;
			vertices[1] = rightTop;
			vertices[2] = leftBottom;
			vertices[3] = rightTop;
			vertices[4] = rightBottom;
			vertices[5] = leftBottom;
		}
	}
}

void XERUIRender::drawStackedBarChart(float32x2 position, float32 height,
	float32 horizontalScale, float32* values, uint32* colors, uint32 valueCount)
{
	float32x2 ndcScaleCoef = getNDCScaleCoef();

	position *= ndcScaleCoef;
	position += float32x2(-1.0f, 1.0f);
	height *= ndcScaleCoef.y;
	horizontalScale *= ndcScaleCoef.x;

	float32 x = position.x;
	float32 top = position.y;
	float32 bottom = top + height;

	VertexUIColor *vertices = to<VertexUIColor*>(allocateVertices(sizeof(VertexUIColor) * valueCount * 6, XERUIGeometryType::Color));
	for (uint32 i = 0; i < valueCount; i++)
	{
		uint32 color = colors[i];

		VertexUIColor leftTop = { { x, top }, color };
		VertexUIColor leftBottom = { { x, bottom }, color };

		x += values[i] * horizontalScale;
		VertexUIColor rightTop = { { x, top }, color };
		VertexUIColor rightBottom = { { x, bottom }, color };

		vertices[0] = leftTop;
		vertices[1] = rightTop;
		vertices[2] = leftBottom;
		vertices[3] = rightTop;
		vertices[4] = rightBottom;
		vertices[5] = leftBottom;

		vertices += 6;
	}
}