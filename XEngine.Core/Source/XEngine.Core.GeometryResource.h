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
		Render::GeometryHandle geometry = Render::GeometryHandle(0);

	public:
		GeometryResource() = default;
		~GeometryResource();

		bool createFromFileAsync(const char* filename);
		void createCube();
		void createSphere();

		void cancelCreation();

		bool isReady() const;
		void setReadyCallback() const;

		inline Render::GeometryHandle getGeometry() const { return geometry; }
	};

	using GeometryResourceManager = AbstractResourceManager<
		GeometryResourceUID, GeometryResourceHandle, GeometryResource>;
}