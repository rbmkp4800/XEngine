#include <d3d12.h>
#include "Util.D3D12.h"

#include <XLib.Util.h>
#include <XLib.Memory.h>

#include "XEngine.Render.Geometry.h"
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