#include <d3d12.h>
#include "Util.D3D12.h"

#include <XLib.Util.h>
#include <XLib.Memory.h>
#include <XLib.Heap.h>

#include "XEngine.Render.Resources.h"
#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine;

void XERGeometry::initialize(XERDevice* device, const void* vertices, uint32 vertexCount,
	uint32 vertexStride, const uint32* indices, uint32 indexCount)
{
	this->vertexCount = vertexCount;
	this->indexCount = indexCount;
	this->vertexStride = vertexStride;
	uint32 vertexBufferSize = vertexCount * vertexStride;
	uint32 indexBufferSize = indexCount * sizeof(uint32);

	device->d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &D3D12ResourceDesc_Buffer(vertexBufferSize + indexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, d3dBuffer.uuid(), d3dBuffer.voidInitRef());

	device->uploadEngine.uploadBuffer(d3dBuffer, 0, vertices, vertexBufferSize);
	device->uploadEngine.uploadBuffer(d3dBuffer, vertexBufferSize, indices, indexBufferSize);
}

void XERUIGeometryBuffer::initialize(XERDevice* device, uint32 size)
{
	device->d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &D3D12ResourceDesc_Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, d3dBuffer.uuid(), d3dBuffer.voidInitRef());
}

void XERUIGeometryBuffer::update(XERDevice* device, uint32 destOffset, const void* data, uint32 size)
{
	device->uploadEngine.uploadBuffer(d3dBuffer, destOffset, data, size);
}

void XERTexture::initialize(XERDevice* device, const void* data, uint32 width, uint32 height)
{
	device->d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height),
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, d3dTexture.uuid(), d3dTexture.voidInitRef());
	device->uploadEngine.uploadTextureAndGenerateMIPMaps(d3dTexture, data, width * 4, to<void*>(data));

	srvDescriptor = device->srvHeap.allocateDescriptors(1);
	device->d3dDevice->CreateShaderResourceView(d3dTexture, nullptr, device->srvHeap.getCPUHandle(srvDescriptor));
}

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
	device->uploadEngine.uploadTextureAndGenerateMIPMaps(d3dTexture, bitmap, textureWidth, bitmap);
}