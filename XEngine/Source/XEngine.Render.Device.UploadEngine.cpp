#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Memory.h>
#include <XLib.Debug.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.h"
#include "XEngine.Render.MIPMapGenerator.h"

using namespace XLib;
using namespace XEngine;

static constexpr uint32 uploadBufferSize = 0x40000;
static constexpr uint32 uploadFragmentMinSize = 0x100;

void XERDevice::UploadEngine::initalize(ID3D12Device* d3dDevice)
{
	copyGPUQueue.initialize(d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, d3dCommandAllocator.uuid(), d3dCommandAllocator.voidInitRef());
	d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, d3dCommandAllocator, nullptr, d3dCommandList.uuid(), d3dCommandList.voidInitRef());
	d3dCommandList->Close();

	d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &D3D12ResourceDesc_Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, d3dUploadBuffer.uuid(), d3dUploadBuffer.voidInitRef());
	d3dUploadBuffer->Map(0, &D3D12Range(), to<void**>(&mappedUploadBuffer));

	d3dCommandAllocator->Reset();
	d3dCommandList->Reset(d3dCommandAllocator, nullptr);
}

void XERDevice::UploadEngine::uploadTextureMIPLevel(DXGI_FORMAT format,
	ID3D12Resource* d3dDestTexture, uint16 mipLevel, uint16 width, uint16 height,
	uint16 pixelPitch, uint32 sourceRowPitch, const void* sourceData)
{
	uint32 rowByteSize = width * pixelPitch;
	uint32 uploadRowPitch = alignup(rowByteSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	for (uint16 rowsUploaded = 0; rowsUploaded < height;)
	{
		uploadBufferBytesUsed = min(alignup(uploadBufferBytesUsed, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT), uploadBufferSize);
		uint16 rowsFitToBuffer = uint16((uploadBufferSize - uploadBufferBytesUsed) / uploadRowPitch);
		uint16 rowsToUpload = min<uint16>(rowsFitToBuffer, height - rowsUploaded);

		for (uint32 i = 0; i < rowsToUpload; i++)
		{
			Memory::Copy(mappedUploadBuffer + uploadBufferBytesUsed + i * uploadRowPitch,
				to<byte*>(sourceData) + (rowsUploaded + i) * sourceRowPitch, rowByteSize);
		}

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = d3dUploadBuffer;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Offset = uploadBufferBytesUsed;
		srcLocation.PlacedFootprint.Footprint.Format = format;
		srcLocation.PlacedFootprint.Footprint.Width = width;
		srcLocation.PlacedFootprint.Footprint.Height = rowsToUpload;
		srcLocation.PlacedFootprint.Footprint.Depth = 1;
		srcLocation.PlacedFootprint.Footprint.RowPitch = uploadRowPitch;

		d3dCommandList->CopyTextureRegion(&D3D12TextureCopyLocation(d3dDestTexture, mipLevel),
			0, rowsUploaded, 0, &srcLocation, &D3D12Box(0, width, 0, rowsToUpload));

		if (rowsToUpload == rowsFitToBuffer)
		{
			flushCommandList();
			uploadBufferBytesUsed = 0;
		}
		else
			uploadBufferBytesUsed += rowsToUpload * uploadRowPitch;

		rowsUploaded += rowsToUpload;
	}
}

void XERDevice::UploadEngine::flushCommandList()
{
	d3dCommandList->Close();

	ID3D12CommandList *d3dCommandListsToExecute[] = { d3dCommandList };
	copyGPUQueue.execute(d3dCommandListsToExecute, countof(d3dCommandListsToExecute));

	d3dCommandAllocator->Reset();
	d3dCommandList->Reset(d3dCommandAllocator, nullptr);
}

void XERDevice::UploadEngine::flushLastBufferUploadToCommandList()
{
	if (d3dLastBufferUploadResource.isInitialized())
	{
		d3dCommandList->CopyBufferRegion(d3dLastBufferUploadResource, lastBufferUploadDestOffset,
			d3dUploadBuffer, uploadBufferBytesUsed - lastBufferUploadSize, lastBufferUploadSize);

		d3dLastBufferUploadResource = nullptr;
		lastBufferUploadDestOffset = 0;
		lastBufferUploadSize = 0;
	}
}

void XERDevice::UploadEngine::flush()
{
	flushLastBufferUploadToCommandList();
	flushCommandList();

	uploadBufferBytesUsed = 0;
}

void XERDevice::UploadEngine::uploadTextureAndGenerateMIPMaps(ID3D12Resource* d3dDestTexture,
	const void* sourceData, uint32 sourceRowPitch, void* mipsGenerationBuffer)
{
	flush();

	D3D12_RESOURCE_DESC desc = d3dDestTexture->GetDesc();
	uint16 pixelPitch = 0;
	switch (desc.Format)
	{
	case DXGI_FORMAT_A8_UNORM: pixelPitch = sizeof(uint8); break;
	case DXGI_FORMAT_R8G8B8A8_UNORM: pixelPitch = sizeof(uint8x4); break;
	default: Debug::Crash(DbgMsgFmt("invalid format"));
	}

	uint16x2 mipSize = { uint16(desc.Width), uint16(desc.Height) };

	Debug::CrashCondition(mipSize.x * pixelPitch > uploadBufferSize, DbgMsgFmt("texture is too large"));

	const void *mipSourceData = sourceData;
	uint32 mipSourceRowPitch = sourceRowPitch;
	uint16 mipLevel = 0;
	for (;;)
	{
		uploadTextureMIPLevel(desc.Format, d3dDestTexture, mipLevel,
			mipSize.x, mipSize.y, pixelPitch, mipSourceRowPitch, mipSourceData);

		mipLevel++;
		if (mipLevel >= desc.MipLevels)
			break;
		if (mipSize.x <= 1 && mipSize.y <= 1)
			break;

		switch (desc.Format)
		{
		case DXGI_FORMAT_A8_UNORM:
			MIPMapGenerator::GenerateLevel<uint8>(mipSourceData, mipSize,
				mipSourceRowPitch, mipsGenerationBuffer, mipSize);
			break;

		case DXGI_FORMAT_R8G8B8A8_UNORM:
			MIPMapGenerator::GenerateLevel<uint8x4>(mipSourceData, mipSize,
				mipSourceRowPitch, mipsGenerationBuffer, mipSize);
			break;
		}

		mipSourceData = to<byte*>(mipsGenerationBuffer);
		mipSourceRowPitch = mipSize.x * pixelPitch;
	}

	flushCommandList();
	uploadBufferBytesUsed = 0;
}

void XERDevice::UploadEngine::uploadBuffer(ID3D12Resource* d3dDestBuffer,
	uint32 destOffset, const void* data, uint32 size)
{
	Debug::CrashCondition(d3dDestBuffer == nullptr, DbgMsgFmt("destination ID3D12Resource is null"));

	uint32 bytesUploaded = 0;
	if (d3dDestBuffer == d3dLastBufferUploadResource &&
		destOffset == lastBufferUploadDestOffset + lastBufferUploadSize)
	{
		if (uploadBufferBytesUsed + size <= uploadBufferSize)
		{
			Memory::Copy(mappedUploadBuffer + uploadBufferBytesUsed, data, size);
			lastBufferUploadSize += size;
			uploadBufferBytesUsed += size;
			return;
		}
		else if (uploadBufferSize - uploadBufferBytesUsed >= uploadFragmentMinSize)
		{
			bytesUploaded = aligndown(uploadBufferSize - uploadBufferBytesUsed, uploadFragmentMinSize);
			Memory::Copy(mappedUploadBuffer + uploadBufferBytesUsed, data, bytesUploaded);
			lastBufferUploadSize += bytesUploaded;
			uploadBufferBytesUsed += bytesUploaded;
		}

		flush();
	}
	else
	{
		flushLastBufferUploadToCommandList();

		if (uploadBufferBytesUsed + size > uploadBufferSize)
		{
			if (uploadBufferSize - uploadBufferBytesUsed >= uploadFragmentMinSize)
			{
				bytesUploaded = aligndown(uploadBufferSize - uploadBufferBytesUsed, uploadFragmentMinSize);
				Memory::Copy(mappedUploadBuffer + uploadBufferBytesUsed, data, bytesUploaded);

				d3dCommandList->CopyBufferRegion(d3dDestBuffer, destOffset,
					d3dUploadBuffer, uploadBufferBytesUsed, bytesUploaded);
			}

			flushCommandList();
			uploadBufferBytesUsed = 0;
		}
	}

	uint32 uploadCount = (size - bytesUploaded) / uploadBufferSize;
	for (uint32 i = 0; i < uploadCount; i++)
	{
		Memory::Copy(mappedUploadBuffer, to<byte*>(data) + bytesUploaded, uploadBufferSize);
		d3dCommandList->CopyBufferRegion(d3dDestBuffer, destOffset + bytesUploaded,
			d3dUploadBuffer, 0, uploadBufferSize);
		flushCommandList();

		bytesUploaded += uploadBufferSize;
	}

	uint32 bytesLeft = size - bytesUploaded;
	if (bytesLeft > 0)
	{
		Memory::Copy(mappedUploadBuffer + uploadBufferBytesUsed, to<byte*>(data) + bytesUploaded, bytesLeft);

		d3dLastBufferUploadResource = d3dDestBuffer;
		lastBufferUploadDestOffset = destOffset + bytesUploaded;
		lastBufferUploadSize = bytesLeft;
		uploadBufferBytesUsed += bytesLeft;
	}
}