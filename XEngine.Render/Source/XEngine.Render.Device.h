#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>
#include <XLib.Color.h>

#include "XEngine.Render.Base.h"
#include "XEngine.Render.Device.DescriptorHeap.h"
#include "XEngine.Render.Device.UploadEngine.h"
#include "XEngine.Render.Device.SceneRenderer.h"
#include "XEngine.Render.Device.BufferHeap.h"
#include "XEngine.Render.Device.TextureHeap.h"
#include "XEngine.Render.Device.EffectHeap.h"
#include "XEngine.Render.Device.MaterialHeap.h"

struct ID3D12Device;
struct IDXGIFactory5;
struct ID3D12DescriptorHeap;

namespace XEngine::Render { class Camera; }
namespace XEngine::Render { class Scene; }
namespace XEngine::Render { class Target; }
namespace XEngine::Render { class GBuffer; }
namespace XEngine::Render { class SwapChain; }

namespace XEngine::Render
{
	using GeometryHeap = Device_::BufferHeap;
	using TextureHeap = Device_::TextureHeap;
	using EffectHeap = Device_::EffectHeap;
	using MaterialHeap = Device_::MaterialHeap;

	class Device : public XLib::NonCopyable
	{
		friend Device_::SceneRenderer;
		friend GBuffer;
		friend SwapChain;

	private:
		XLib::Platform::COMPtr<ID3D12Device> d3dDevice;
		static XLib::Platform::COMPtr<IDXGIFactory5> dxgiFactory;

		Device_::DescriptorHeap srvHeap;
		Device_::DescriptorHeap rtvHeap;
		Device_::DescriptorHeap dsvHeap;

		Device_::SceneRenderer sceneRenderer;

		Device_::UploadEngine uploadEngine;

		Device_::BufferHeap		bufferHeap;
		Device_::TextureHeap	textureHeap;
		Device_::EffectHeap		effectHeap;
		Device_::MaterialHeap	materialHeap;
		
	private:
		

	public:
		Device() = default;
		~Device() = default;

		bool initialize();
		void destroy();

		void clearTarget(Target& target, XLib::Color color);
		void renderScene(Scene& scene, const Camera& camera, GBuffer& gBuffer,
			Target& target, rectu16 viewport);

		inline GeometryHeap&	getBufferHeap()		{ return bufferHeap;   }
		inline TextureHeap&		getTextureHeap()	{ return textureHeap;  }
		inline EffectHeap&		getEffectHeap()		{ return effectHeap;   }
		inline MaterialHeap&	getMaterialHeap()	{ return materialHeap; }
	};
}