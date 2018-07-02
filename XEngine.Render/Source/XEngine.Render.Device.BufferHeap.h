#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.Render.Base.h"

namespace XEngine::Render { class Device; }

namespace XEngine::Render::Device_
{
	class BufferHeap final : public XLib::NonCopyable
	{
		friend Device;

	private:
		BufferHeap() = default;
		~BufferHeap() = default;

		inline Device* getDevice();

		void initialize();

	public:
		BufferHandle createBuffer(uint32 size);
		void releaseBuffer(BufferHandle handle);

		uint64 getBufferGPUAddress(BufferHandle handle) const;
	};
}