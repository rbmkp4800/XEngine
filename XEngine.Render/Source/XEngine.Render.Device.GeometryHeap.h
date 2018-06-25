#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.Render.Base.h"

namespace XEngine::Render::Device_
{
	struct GeometryDesc
	{
		uint64 vertexBufferGPUAddress;
		uint64 indexBufferGPUAddress;
		uint8 vertexStride;
		bool indexIs32Bit;
	};

	class GeometryHeap final : public XLib::NonCopyable
	{
		friend Device;

	private:
		GeometryHeap() = default;
		~GeometryHeap() = default;

		inline Device* getDevice();

		void initialize();

	public:

		inline const GeometryDesc& getGeometryDesc(GeometryHandle handle) const;
	};
}