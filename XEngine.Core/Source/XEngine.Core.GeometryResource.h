#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include <XEngine.Render.Base.h>

#include "XEngine.Core.AbstractResourceManager.h"

namespace XEngine::Core
{
	using GeometryResourceUID = uint64;
	using GeometryResourceHandle = uint32;

	class GeometryResource : public XLib::NonCopyable
	{
	private:
		Render::BufferHandle buffer = Render::BufferHandle(0);
		uint32 vertexCount = 0;
		uint32 indexCount = 0;
		uint8 vertexStride = 0;
		bool indexIs32Bit = false;

	public:
		GeometryResource() = default;
		~GeometryResource();

		bool createFromFileAsync(const char* filename);
		void createCube();
		void createSphere();

		void cancelCreation();

		bool isReady() const;
		void setReadyCallback() const;

		inline Render::BufferHandle getBufferHandle() const { return buffer; }
	};

	using GeometryResourceManager = AbstractResourceManager<
		GeometryResourceUID, GeometryResourceHandle, GeometryResource>;
}