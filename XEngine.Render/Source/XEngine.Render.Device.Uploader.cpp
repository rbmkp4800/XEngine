#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Memory.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.Uploader.h"

#include "XEngine.Render.Device.h"
#include "XEngine.Render.ClassLinkage.h"

#define device this->getDevice()

using namespace XLib;
using namespace XEngine::Render::Device_;

static constexpr uint32 uploadBufferSize = 0x40000;

void Uploader::initialize()
{
	ID3D12Device *d3dDevice = device.d3dDevice;

	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, d3dCommandAllocator.uuid(), d3dCommandAllocator.voidInitRef());
	d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, d3dCommandAllocator, nullptr, d3dCommandList.uuid(), d3dCommandList.voidInitRef());
	d3dCommandList->Close();

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dUploadBuffer.uuid(), d3dUploadBuffer.voidInitRef());
	d3dUploadBuffer->Map(0, &D3D12Range(), to<void**>(&mappedUploadBuffer));

	gpuQueueSyncronizer.initialize(d3dDevice);
}

void Uploader::destroy()
{

}

void Uploader::uploadBuffer(ID3D12Resource* d3dDestBuffer,
	uint32 destOffset, const void* data, uint32 size)
{
	for (uint32 bytesUploaded = 0; bytesUploaded < size; bytesUploaded += uploadBufferSize)
	{
		uint32 currentUploadSize = min(size - bytesUploaded, uploadBufferSize);

		Memory::Copy(mappedUploadBuffer, to<byte*>(data) + bytesUploaded, currentUploadSize);

		d3dCommandAllocator->Reset();
		d3dCommandList->Reset(d3dCommandAllocator, nullptr);

		d3dCommandList->CopyBufferRegion(d3dDestBuffer, destOffset + bytesUploaded,
			d3dUploadBuffer, 0, currentUploadSize);

		d3dCommandList->Close();

		ID3D12CommandList *d3dCommandListsToExecute[] = { d3dCommandList };
		device.d3dCopyQueue->ExecuteCommandLists(
			countof(d3dCommandListsToExecute), d3dCommandListsToExecute);

		gpuQueueSyncronizer.synchronize(device.d3dCopyQueue);
	}
}