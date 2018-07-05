#include <d3d12.h>

#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.BufferHeap.h"

#include "XEngine.Render.Device.h"

#define device this->getDevice()

using namespace XEngine::Render;
using namespace XEngine::Render::Device_;

void BufferHeap::initialize()
{

}

BufferHandle BufferHeap::createBuffer(uint32 size)
{
	XLib::Platform::COMPtr<ID3D12Resource>& d3dBuffer = d3dBuffers[bufferCount];

	device.d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dBuffer.uuid(), d3dBuffer.voidInitRef());

	BufferHandle result = BufferHandle(bufferCount);
	bufferCount++;
	return result;
}

uint64 BufferHeap::getBufferGPUAddress(BufferHandle handle)
{
	return d3dBuffers[handle]->GetGPUVirtualAddress();
}