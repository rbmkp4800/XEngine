#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include <XEngine.Render.Base.h>
#include <XEngine.Render.Scene.h>

#include "XEngine.Core.AbstractResourceManager.h"

namespace XEngine::Core
{
	using GeometryResourceUID = uint64;
	using GeometryResourceHandle = uint16;

	class GeometryResource : public XLib::NonCopyable
	{
	private:
		uint32 vertexCount = 0;
		uint32 indexCount = 0;
        Render::BufferHandle buffer = Render::BufferHandle(0);
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

		inline Render::GeometryDesc getGeometryDesc() const
		{
			Render::GeometryDesc result;
			result.vertexBufferHandle = buffer;
			result.indexBufferHandle = buffer;
			result.vertexDataOffset = 0;
			result.indexDataOffset = indexCount * (indexIs32Bit ? 4 : 2);
			result.vertexCount = vertexCount;
			result.indexCount = indexCount;
			result.vertexStride = vertexStride;
			result.indexIs32Bit = indexIs32Bit;
			return result;
		}
	};

	using GeometryResourceManager = AbstractResourceManager<
		GeometryResourceUID, GeometryResourceHandle, GeometryResource>;
}