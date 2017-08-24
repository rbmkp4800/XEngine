#include <d3d12.h>
#include "Util.D3D12.h"

#include <XLib.Util.h>
#include <XLib.Vectors.Arithmetics.h>

#include "XEngine.Render.UIRender.h"
#include "XEngine.Render.Resources.h"
#include "XEngine.Render.Targets.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Vertices.h"

using namespace XLib;
using namespace XEngine;

static constexpr uint32 vertexBufferSize = 0x8000;

void XERUIRender::flushCurrentGeometry()
{
	if (currentGeometryVertexBufferOffset == vertexBufferUsedBytes)
		return;

	if (!commandListInitialized)
		initCommandList();

	ID3D12PipelineState *d3dPS = nullptr;
	uint32 vertexStride = 0;
	switch (currentGeometryType)
	{
		case GeometryType::Color:
			vertexStride = sizeof(VertexUIColor);
			d3dPS = device->d3dUIColorPSO;
			break;

		case GeometryType::Font:
			vertexStride = sizeof(VertexUIFont);
			d3dPS = device->d3dUIFontPSO;
			break;

		default: throw;
	}

	uint32 currentGeometryVertexBufferSize = vertexBufferUsedBytes - currentGeometryVertexBufferOffset;

	d3dCommandList->SetPipelineState(d3dPS);
	if (currentGeometrySRVDescriptor != uint32(-1))
		d3dCommandList->SetGraphicsRootDescriptorTable(0, device->srvHeap.getGPUHandle(currentGeometrySRVDescriptor));
	d3dCommandList->IASetVertexBuffers(0, 1, &D3D12VertexBufferView(
		d3dVertexBuffer->GetGPUVirtualAddress() + currentGeometryVertexBufferOffset,
		currentGeometryVertexBufferSize, vertexStride));
	d3dCommandList->DrawInstanced(currentGeometryVertexBufferSize / vertexStride, 1, 0, 0);

	currentGeometryVertexBufferOffset = vertexBufferUsedBytes;
	currentGeometryType = GeometryType::None;
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

// internal interface =======================================================================//

void* XERUIRender::allocateVertexBuffer(uint32 size, GeometryType geometryType)
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

void XERUIRender::setSRVDescriptor(uint32 srvDescriptor)
{
	if (currentGeometrySRVDescriptor != srvDescriptor)
	{
		flushCurrentGeometry();
		currentGeometrySRVDescriptor = srvDescriptor;
	}
}

// public interface =========================================================================//

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

void XERUIRender::drawText(XERMonospacedFont* font, float32x2 position,
	const char* text, uint32 color, uint32 length)
{
	setSRVDescriptor(font->srvDescriptor);

	uint8 firstCharCode = font->firstCharCode;
	uint8 lastCharCode = firstCharCode + font->charTableSize - 1;
	uint32 charTableSize = font->charTableSize;

	float32x2 ndcScaleCoef(1.0f / float32(targetWidth / 2), -1.0f / float32(targetHeight / 2));

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

			VertexUIFont *vertices = to<VertexUIFont*>(allocateVertexBuffer(sizeof(VertexUIFont) * 6, GeometryType::Font));

			vertices[0] = leftTop;
			vertices[1] = rightTop;
			vertices[2] = leftBottom;
			vertices[3] = rightTop;
			vertices[4] = rightBottom;
			vertices[5] = leftBottom;
		}
	}
}