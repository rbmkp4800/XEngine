#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Internal.GPUQueueSynchronizer.h"

// NOTE: temporary implementation

struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;
struct ID3D12Resource;

namespace XEngine::Render { class Device; }

namespace XEngine::Render::Device_
{
	class Uploader : public XLib::NonCopyable
	{
	private:
		XLib::Platform::COMPtr<ID3D12GraphicsCommandList> d3dCommandList;
		XLib::Platform::COMPtr<ID3D12CommandAllocator> d3dCommandAllocator;
		XLib::Platform::COMPtr<ID3D12Resource> d3dUploadBuffer;
		byte *mappedUploadBuffer = nullptr;

		Internal::GPUQueueSynchronizer gpuQueueSyncronizer;

	private:
		inline Device& getDevice();

	public:
		Uploader() = default;
		~Uploader() = default;

		void initialize();
		void destroy();

		void uploadBuffer(ID3D12Resource* d3dDestBuffer,
			uint32 destOffset, const void* data, uint32 size);
	};
}