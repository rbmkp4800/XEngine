#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

// NOTE: temporary implementation

struct ID3D12Resource;

namespace XEngine::Render { class Device; }

namespace XEngine::Render::Device_
{
	class BufferHeap : public XLib::NonCopyable
	{
	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dBuffers[16];
		uint32 bufferCount = 0;

	private:
		inline Device& getDevice();

	public:
		BufferHeap() = default;
		~BufferHeap() = default;

		void initialize();

		BufferHandle createBuffer(uint32 size);
		void releaseBuffer(BufferHandle handle);

		uint64 getBufferGPUAddress(BufferHandle handle);
		ID3D12Resource* getD3DResource(BufferHandle handle);
	};
}