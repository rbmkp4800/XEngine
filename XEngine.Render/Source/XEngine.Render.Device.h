#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"
#include "XEngine.Render.Device.SceneRendererResources.h"
#include "XEngine.Render.Device.GeometryHeap.h"
#include "XEngine.Render.Device.TextureHeap.h"
#include "XEngine.Render.Device.EffectHeap.h"
#include "XEngine.Render.Device.MaterialHeap.h"

struct ID3D12Device;

namespace XEngine::Render
{
	using GeometryHeap = Device_::GeometryHeap;
	using TextureHeap = Device_::TextureHeap;
	using EffectHeap = Device_::EffectHeap;
	using MaterialHeap = Device_::MaterialHeap;

	class Device : public XLib::NonCopyable
	{
		friend SceneRenderer;

	private:
		XLib::Platform::COMPtr<ID3D12Device> d3dDevice;

		Device_::SceneRendererResources sceneRendererResources;

		Device_::GeometryHeap geometryHeap;
		Device_::TextureHeap textureHeap;
		Device_::EffectHeap effectHeap;
		Device_::MaterialHeap materialHeap;

	public:
		Device() = default;
		~Device() = default;

		void initialize();
		void destroy();

		inline GeometryHeap&	getGeometryHeap()	{ return geometryHeap; }
		inline TextureHeap&		getTextureHeap()	{ return textureHeap;  }
		inline EffectHeap&		getEffectHeap()		{ return effectHeap;   }
		inline MaterialHeap&	getMaterialHeap()	{ return materialHeap; }
	};
}