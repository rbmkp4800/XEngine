#pragma once

#include <XLib.Types.h>

#include <XEngine.Render.Base.h>

#include "XEngine.Core.AbstractResourceManager.h"

namespace XEngine::Core
{
	using GeometryResourceUID = uint64;
	using GeometryResourceHandle = uint32;

	struct GeometryResource
	{
	private:
		Render::GeometryHandle geometry;

	public:
		bool isReady() const;
		void setReadyCallback() const;
	};

	class GeometryFactory
	{
	public:
		void createFromFile(const char* filename);
	};

	using GeometryManager = AbstractResourceManager<
		GeometryResourceUID, GeometryResourceHandle, GeometryResource>;
}