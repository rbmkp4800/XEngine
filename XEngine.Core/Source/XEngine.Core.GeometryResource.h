#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XEngine.Render.Base.h>
#include <XEngine.Render.Scene.h>

#include "XEngine.Core.ResourceCache.h"

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
		~GeometryResource() = default;

		// NOTE: temporary
		bool createFromFile(const char* filename);
		void createCube();
		void createCubicSphere(uint32 detalizationLevel);

		inline Render::GeometryDesc getGeometryDesc() const
		{
			Render::GeometryDesc result;
			result.vertexBufferHandle = buffer;
			result.indexBufferHandle = buffer;
			result.vertexDataOffset = 0;
			result.indexDataOffset = vertexCount * vertexStride;
			result.vertexDataSize = result.indexDataOffset;
			result.indexCount = indexCount;
			result.vertexStride = vertexStride;
			result.indexIs32Bit = indexIs32Bit;
			return result;
		}

	public: // creation
		enum class CreationType : uint8;
		struct CreationArgs;
		class CreationTask;

		static bool Create(const CreationArgs& args, CreationTask& task);
	};

	enum class GeometryResource::CreationType : uint8
	{
		None = 0,
		Cube,
		CubicSphere,
		FromFile,
	};

	struct GeometryResource::CreationArgs
	{
		CreationType type = CreationType::None;

		union
		{
			struct
			{
				uint16 detalization;
			} cubicSphere;
			struct
			{
				char filename[32];
			} fromFile;
		};
	};

	class GeometryResource::CreationTask : public XLib::NonCopyable
	{
	private:

	public:
		CreationTask() = default;
		~CreationTask();

		bool cancel();
	};

	//using GeometryResourceCache = ResourceCache<
	//	GeometryResourceUID, GeometryResourceHandle, GeometryResource>;
}